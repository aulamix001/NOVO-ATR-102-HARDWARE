#pragma once
#include "TapeConfig.h"
#include <cstdint>
#include <vector>
namespace reelglue {
inline double dbToGain(double db) { return std::pow(10.0, db/20.0); }
struct Biquad {
    double b0=1,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;
    void reset() { z1=z2=0; }
    double process(double x) { auto y=b0*x+z1; z1=b1*x-a1*y+z2; z2=b2*x-a2*y; return y; }
    void shelf(double fs,double hz,double db,bool high) {
        const double A=std::pow(10.0,db/40.0),w=2*constants::pi*std::min(hz,fs*0.45)/fs;
        const double cs=std::cos(w),ss=std::sin(w),alpha=ss/std::sqrt(2.0),beta=2*std::sqrt(A)*alpha;
        double den;
        if(high){den=(A+1)-(A-1)*cs+beta;b0=A*((A+1)+(A-1)*cs+beta);b1=-2*A*((A-1)+(A+1)*cs);b2=A*((A+1)+(A-1)*cs-beta);a1=2*((A-1)-(A+1)*cs);a2=(A+1)-(A-1)*cs-beta;}
        else{den=(A+1)+(A-1)*cs+beta;b0=A*((A+1)-(A-1)*cs+beta);b1=2*A*((A-1)-(A+1)*cs);b2=A*((A+1)-(A-1)*cs-beta);a1=-2*((A-1)+(A+1)*cs);a2=(A+1)+(A-1)*cs-beta;}
        b0/=den;b1/=den;b2/=den;a1/=den;a2/=den;
    }
    void emphasis(double fs,int kind,bool inverse) {
        const double K=2*fs;
        const double lf=constants::emphasisLfSeconds[kind]>0?constants::emphasisLfSeconds[kind]:1/(2*constants::pi*fs*0.45),hf=constants::emphasisHfSeconds[kind];
        double numerator=lf,denominator=hf;if(inverse)std::swap(numerator,denominator);
        // A finite ultrasonic pole keeps LF-free inverse curves proper.
        if(denominator==0)denominator=1/(2*constants::pi*fs*0.45);
        b0=(1+K*numerator)/(1+K*denominator);b1=(1-K*numerator)/(1+K*denominator);b2=0;
        a1=(1-K*denominator)/(1+K*denominator);a2=0;
        const double w=2*constants::pi*1000/fs,cs=std::cos(w);
        const double magnitude=std::sqrt((b0*b0+b1*b1+2*b0*b1*cs)/(1+a1*a1+2*a1*cs));
        b0/=magnitude;b1/=magnitude;
    }
    void peak(double fs,double hz,double db,double q) {
        const auto w=2*constants::pi*std::clamp(hz,10.0,fs*0.45)/fs;
        const auto alpha=std::sin(w)/(2*q), A=std::pow(10.0,db/40.0), den=1+alpha/A;
        b0=(1+alpha*A)/den; b1=-2*std::cos(w)/den; b2=(1-alpha*A)/den;
        a1=b1; a2=(1-alpha/A)/den;
    }
};
struct Settings {
    TapeConfig config;
    std::array<double,5> calibration {0.5,0.5,0.5,0.5,0.5};
    double record=0,reproduce=0,mix=1;
    bool power=true,thru=false,transformer=true,noise=false,wow=true;
    int path=1,quality=1;
};
class TapeChannel {
public:
    void prepare(double sampleRate,int channel) {
        fs=sampleRate; delay.assign(static_cast<size_t>(std::ceil(fs*0.002))+16,0);
        dryDelay.assign(delay.size(),0); base=static_cast<int>(std::ceil(fs*constants::wowBaseSeconds));
        seed=1234567u+static_cast<uint32_t>(channel)*67891u;
        smooth=1-std::exp(-1/(fs*constants::smoothingSeconds));
        meterRise=1-std::exp(-1/(fs*constants::meterRiseSeconds));
        meterFall=1-std::exp(-1/(fs*constants::meterFallSeconds));
        reset();
    }
    void reset() { std::fill(delay.begin(),delay.end(),0); std::fill(dryDelay.begin(),dryDelay.end(),0);
        write=0; phase=0; bump.reset(); recordEq.reset(); replayEq.reset(); hfEq.reset(); shelfEq.reset(); reproHf.reset(); reproLf.reset(); biasEq.reset(); low=0; hiss=0; meter=0; clip=0; }
    int latency() const {return base;}
    void configure(const Settings& s) {
        settings=s; ideal=idealFor(s.config);recTarget=dbToGain(s.record);repTarget=dbToGain(s.reproduce);
        bump.peak(fs,s.config.ips*0.0254/constants::headWavelengthMetres,constants::headBumpDb+0.3*s.config.head,constants::headBumpQ);
        cutoff=1-std::exp(-2*constants::pi*std::min(fs*0.45,12000.0*std::sqrt(s.config.ips/15.0))/fs);
        recordEq.emphasis(fs,s.config.emphasis,true);replayEq.emphasis(fs,s.config.emphasis,false);
        hfEq.shelf(fs,constants::calibrationHfHz,(s.calibration[0]-ideal[0])*2*constants::calibrationHfDb,true);
        shelfEq.shelf(fs,constants::calibrationShelfHz,(s.calibration[1]-ideal[1])*2*constants::calibrationLfDb,false);
        reproHf.shelf(fs,constants::calibrationHfHz,(s.calibration[2]-ideal[2])*2*constants::calibrationHfDb,true);
        reproLf.shelf(fs,constants::calibrationShelfHz,(s.calibration[3]-ideal[3])*2*constants::calibrationLfDb,false);
        biasEq.shelf(fs,constants::calibrationHfHz,-(s.calibration[4]-ideal[4])*2*constants::biasHfDb,true);
        noiseTarget=s.noise ? dbToGain(constants::noiseDbfs-3*std::log2(s.config.ips/15.0)-3*s.config.head-20*std::log10(s.config.referenceFlux/355.0)):0;
        drive=constants::saturationDrive/(constants::flux[constants::recommendedCal[s.config.formula]]*std::sqrt(10.0));
        drive*=dbToGain(-0.8*(s.calibration[4]-ideal[4])*2);
    }
    float process(float input) noexcept {
        rec+=(recTarget-rec)*smooth; rep+=(repTarget-rep)*smooth;
        wet+=((settings.power&&!settings.thru ? settings.mix:0)-wet)*smooth;
        noiseLevel+=(noiseTarget-noiseLevel)*smooth;
        double v=input/constants::referenceAmplitude*rec;
        clip=std::max(0,clip-1);
        if (std::abs(v)>constants::railLimit) clip=static_cast<int>(fs*0.25);
        v=std::clamp(v,-constants::railLimit,constants::railLimit);
        transformerMix+=((settings.transformer?1.0:0.0)-transformerMix)*smooth;
        v+=(std::tanh(v*0.08)/0.08-v)*transformerMix;
        if(settings.path!=2) {
            v=biasEq.process(shelfEq.process(hfEq.process(recordEq.process(v))));
            double flux=v*settings.config.referenceFlux;
            double x=flux*drive;
            auto asym=std::clamp((settings.calibration[4]-ideal[4])*-0.12,-0.12,0.12);
            flux=std::tanh(x+asym*x*x)/drive;
            v=flux/settings.config.referenceFlux;
            low+=cutoff*(v-low); v=bump.process(low);
            v=reproLf.process(reproHf.process(replayEq.process(v)));
        }
        auto value=v*constants::referenceAmplitude;
        delay[static_cast<size_t>(write)]=value;
        dryDelay[static_cast<size_t>(write)]=input;
        wowMix+=((settings.wow&&settings.path!=2?1.0:0.0)-wowMix)*smooth;
        double mod=wowMix*(constants::wowDepthSeconds*std::sin(phase)+constants::flutterDepthSeconds*std::sin(phase*26));
        const double scale=settings.config.speed==3 ? 0.7:constants::speedWowScale[settings.config.speed];
        double position=write-base-mod*fs*scale;
        while(position<0)position+=delay.size();
        const auto n=static_cast<int>(position);const auto f=position-n;
        const auto sz=static_cast<int>(delay.size());
        auto at=[&](int offset) noexcept {return delay[static_cast<size_t>((n+offset+sz)%sz)];};
        const double ym=at(-1),y0=at(0),y1=at(1),y2=at(2);
        const double a=0.5*(y1-ym),b=ym-2.5*y0+2*y1-0.5*y2,c=0.5*(y2-ym)+1.5*(y0-y1);
        value=((c*f+b)*f+a)*f+y0;
        phase+=2*constants::pi*constants::wowHz/fs;if(phase>2*constants::pi)phase-=2*constants::pi;
        seed=seed*1664525u+1013904223u;
        const double white=static_cast<double>(seed)/2147483648.0-1;
        hiss=0.97*hiss+0.03*white;
        value+=(hiss*7.0+0.2*std::sin(phase*settings.config.humHz/constants::wowHz))*noiseLevel;
        value*=rep;
        const int dryIndex=(write-base+sz)%sz;
        auto out=dryDelay[static_cast<size_t>(dryIndex)]*(1-wet)+value*wet;
        write=(write+1)%sz;
        if(!std::isfinite(out)){reset();return 0;}
        const double square=out*out;
        const double coeff=square>meter?meterRise:meterFall;
        meter+=coeff*(square-meter);
        return static_cast<float>(out);
    }
    double level() const {return std::sqrt(std::max(0.0,meter));}
    bool clipping() const {return clip>0;}
private:
    double fs=48000,smooth=0.001,rec=1,rep=1,recTarget=1,repTarget=1,meterRise=0.001,meterFall=0.001,wet=1,cutoff=0.5,low=0,phase=0;
    double noiseLevel=0,noiseTarget=0,drive=0.0005,hiss=0,meter=0,transformerMix=1,wowMix=1;
    int write=0,base=24,clip=0; uint32_t seed=1234567;
    Settings settings; std::array<double,5> ideal {};
    Biquad bump,recordEq,replayEq,hfEq,shelfEq,reproHf,reproLf,biasEq; std::vector<double> delay,dryDelay;
};
}
