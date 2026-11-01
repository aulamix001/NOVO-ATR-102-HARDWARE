#pragma once
#include <JuceHeader.h>
namespace hardware {
inline juce::Colour colour(juce::uint32 c) {return juce::Colour(c);}
inline const auto ink=colour(0xff1d1c1b), cream=colour(0xffd8d5c5), amber=colour(0xffe8b34f), muted=colour(0xff969594);
inline void gradient(juce::Graphics& g,juce::Rectangle<float> r,juce::Colour top,juce::Colour bottom,bool horizontal=false) {
    g.setGradientFill(juce::ColourGradient(top,r.getX(),r.getY(),bottom,horizontal?r.getRight():r.getX(),horizontal?r.getY():r.getBottom(),false));g.fillRect(r);
}
inline void text(juce::Graphics& g,const juce::String& t,juce::Rectangle<float> r,float size,juce::Colour c=cream,juce::Justification align=juce::Justification::centred,bool bold=false) {
    g.setColour(c);g.setFont(juce::Font(juce::FontOptions(size,bold?juce::Font::bold:juce::Font::plain)));g.drawText(t,r,align);
}
inline void screw(juce::Graphics& g,float x,float y,float r=4) {
    g.setColour(juce::Colours::black.withAlpha(0.7f));g.fillEllipse(x-r,y-r+1,2*r,2*r);
    g.setGradientFill(juce::ColourGradient(colour(0xff8a8980),x-r,y-r,colour(0xff262524),x+r,y+r,false));g.fillEllipse(x-r,y-r,2*r,2*r);
    g.setColour(colour(0xff171615));g.drawLine(x-r*0.6f,y+r*0.25f,x+r*0.6f,y-r*0.25f,1.4f);
    g.setColour(colour(0xffa1a196).withAlpha(0.45f));g.drawLine(x-r*0.6f,y+r*0.25f+1,x+r*0.6f,y-r*0.25f+1,0.6f);
}
inline void led(juce::Graphics& g,float x,float y,bool on,juce::Colour c=colour(0xffb6d46d),float r=3) {
    if(on){g.setGradientFill(juce::ColourGradient(c.withAlpha(0.35f),x,y,juce::Colours::transparentBlack,x+r*3,y,true));g.fillEllipse(x-r*3,y-r*3,6*r,6*r);}
    g.setColour(colour(0xff1a1918));g.fillEllipse(x-r-1,y-r-1,2*r+2,2*r+2);
    g.setColour(on?c:colour(0xff4c4b4a));g.fillEllipse(x-r,y-r,2*r,2*r);
    if(on){g.setColour(juce::Colours::white.withAlpha(0.55f));g.fillEllipse(x-r*0.4f,y-r*0.7f,r*0.7f,r*0.6f);}
}
class Look final:public juce::LookAndFeel_V4 {
public:
    void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float position,float start,float end,juce::Slider& slider)override {
        const auto r=juce::Rectangle<float>(static_cast<float>(x),static_cast<float>(y),static_cast<float>(w),static_cast<float>(h)).reduced(4);
        const float radius=std::min(r.getWidth(),r.getHeight())*0.5f-5,cx=r.getCentreX(),cy=r.getCentreY();
        const bool silver=slider.getProperties().getWithDefault("silver",false);
        const auto bounds=juce::Rectangle<float>(cx-radius,cy-radius,radius*2,radius*2);
        for(int i=0;i<=10;++i){const auto a=start+(end-start)*i/10.f;const auto outer=radius+4,inner=radius+2;
            g.setColour(colour(0xff969594));g.drawLine(cx+std::sin(a)*inner,cy-std::cos(a)*inner,cx+std::sin(a)*outer,cy-std::cos(a)*outer,1);}
        juce::DropShadow(juce::Colours::black.withAlpha(0.85f),5,{1,4}).drawForRectangle(g,bounds.toNearestInt().reduced(3));
        g.setColour(colour(0xff080909));g.fillEllipse(bounds.expanded(1.5f));
        g.setGradientFill(juce::ColourGradient(colour(silver?0xffe8e7e6:0xff636261),cx-radius,cy-radius,colour(silver?0xff5d5c5b:0xff131211),cx+radius,cy+radius,false));g.fillEllipse(bounds);
        const auto face=bounds.reduced(radius*0.13f);
        g.setGradientFill(juce::ColourGradient(colour(silver?0xffbebdbc:0xff141312),cx-radius*0.4f,cy-radius,colour(silver?0xffe5e4e3:0xff3c3b3a),cx+radius,cy+radius,false));g.fillEllipse(face);
        g.setColour(juce::Colours::white.withAlpha(0.08f));g.drawEllipse(face,0.8f);
        if(!silver)for(int i=0;i<40;++i){auto a=i*juce::MathConstants<float>::twoPi/40;g.setColour(juce::Colours::black.withAlpha(0.28f));g.drawLine(cx+std::sin(a)*radius*0.89f,cy-std::cos(a)*radius*0.89f,cx+std::sin(a)*radius*0.98f,cy-std::cos(a)*radius*0.98f,1);}
        const auto a=start+position*(end-start);
        g.setColour(juce::Colours::black.withAlpha(0.5f));g.drawLine(cx+std::sin(a)*radius*0.24f+1,cy-std::cos(a)*radius*0.24f+1,cx+std::sin(a)*radius*0.76f+1,cy-std::cos(a)*radius*0.76f+1,3);
        g.setColour(silver?colour(0xff383736):cream);g.drawLine(cx+std::sin(a)*radius*0.24f,cy-std::cos(a)*radius*0.24f,cx+std::sin(a)*radius*0.76f,cy-std::cos(a)*radius*0.76f,2.4f);
    }
};
class Key final:public juce::Button {
public:
    Key(juce::String label,bool toggle=true):Button(label){setClickingTogglesState(toggle);setWantsKeyboardFocus(true);setTooltip(label);}
    bool red=false,dark=false,compact=false;
    void paintButton(juce::Graphics& g,bool over,bool down)override {
        const auto r=getLocalBounds().toFloat();const bool active=getToggleState();
        if(compact){gradient(g,r.reduced(1),colour(active?0xff494847:0xff363534),colour(0xff1f1e1d));g.setColour(colour(0xff121110));g.drawRect(r.reduced(0.5f));hardware::text(g,getButtonText(),r,11,active?amber:muted);return;}
        const float cx=r.getCentreX();
        hardware::text(g,getButtonText(),{0,0,r.getWidth(),12},9,dark?muted:colour(0xff484746),juce::Justification::centred,true);
        led(g,cx,18,active,red?colour(0xfff05a34):colour(0xffb6d46d),2.5f);
        auto key=r.withTrimmedTop(24).reduced(4,0).withHeight(21);
        g.setColour(colour(0xff272625));g.fillRoundedRectangle(key.expanded(2),1.5f);
        g.setColour(juce::Colours::black.withAlpha(0.5f));g.fillRect(key.translated(1,2));
        gradient(g,key,colour(red?0xffe94b25:(down||active?0xffcac9b8:0xfff1efe0)),colour(red?0xffa92110:0xffb6b5a7));
        g.setColour(juce::Colours::white.withAlpha(over?0.6f:0.3f));g.drawLine(key.getX()+1,key.getY()+1,key.getRight()-1,key.getY()+1,1);
        if(hasKeyboardFocus(true)){g.setColour(amber);g.drawRect(key.expanded(3),1);}
    }
};
}
