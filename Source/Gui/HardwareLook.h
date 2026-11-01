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
// Multi-stop lighting is fixed to the chassis: the pointer rotates, the light does not.
inline juce::ColourGradient metalLight(juce::Rectangle<float> r, bool silver) {
    juce::ColourGradient light(colour(silver ? 0xfff5f4ef : 0xff73746f),r.getX(),r.getY(),
                               colour(silver ? 0xff96978f : 0xff0d0e0c),r.getRight(),r.getBottom(),false);
    light.addColour(0.20,colour(silver ? 0xffb7bab2 : 0xff454743));
    light.addColour(0.42,colour(silver ? 0xffe0e2da : 0xff272925));
    light.addColour(0.62,colour(silver ? 0xff858981 : 0xff121411));
    light.addColour(0.80,colour(silver ? 0xffc5c8bf : 0xff30332e));
    return light;
}
inline void keycap(juce::Graphics& g,juce::Rectangle<float> well,bool pressed,bool red,bool hover) {
    // Rolled metal escutcheon, black socket and the key's vertical side wall.
    g.setGradientFill(metalLight(well.expanded(1),true));
    g.fillRoundedRectangle(well.expanded(1),2.6f);
    g.setColour(colour(0xff11130f));g.fillRoundedRectangle(well,2.1f);
    g.setColour(juce::Colours::black.withAlpha(0.8f));g.fillRoundedRectangle(well.reduced(1).translated(0,1),1.8f);
    auto body=well.reduced(3.0f,2.8f).translated(0,pressed ? 1.5f : -1.2f);
    g.setColour(colour(red ? 0xff6b1e0e : 0xff6b6a5c));
    g.fillRoundedRectangle(body.translated(0,pressed ? 1.f : 3.f),1.8f);
    juce::ColourGradient plastic(colour(red ? 0xffff7a43 : 0xfff9f7e6),body.getX(),body.getY(),
                                 colour(red ? 0xffb52c13 : 0xffb5b4a2),body.getX(),body.getBottom(),false);
    plastic.addColour(0.12,colour(red ? 0xffef5026 : 0xffe8e7d5));
    plastic.addColour(0.45,colour(red ? 0xffd53b18 : 0xffd9d9c6));
    plastic.addColour(0.80,colour(red ? 0xffcf3617 : 0xffceceba));
    if(pressed)plastic.multiplyOpacity(0.91f);
    g.setGradientFill(plastic);g.fillRoundedRectangle(body,1.8f);
    // Convex molded edge and fine top bevel catch the overhead light.
    g.setColour(colour(red ? 0xffffb18a : 0xffffffef).withAlpha(pressed ? 0.35f : 0.75f));
    g.drawLine(body.getX()+2,body.getY()+1,body.getRight()-2,body.getY()+1,0.8f);
    g.setColour(juce::Colours::white.withAlpha(0.26f));
    g.drawLine(body.getX()+1,body.getY()+2,body.getX()+1,body.getBottom()-2,0.7f);
    g.setColour(colour(red ? 0xff781f0c : 0xff888976));
    g.drawLine(body.getX()+2,body.getBottom()-0.5f,body.getRight()-2,body.getBottom()-0.5f,0.8f);
    g.drawLine(body.getRight()-0.5f,body.getY()+2,body.getRight()-0.5f,body.getBottom()-1,0.7f);
    if(hover){g.setColour(juce::Colours::white.withAlpha(0.07f));g.fillRoundedRectangle(body,1.8f);}
}
class Look final:public juce::LookAndFeel_V4 {
public:
    void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float position,float start,float end,juce::Slider& slider)override {
        const auto r=juce::Rectangle<float>(static_cast<float>(x),static_cast<float>(y),static_cast<float>(w),static_cast<float>(h)).reduced(4);
        const float radius=std::min(r.getWidth(),r.getHeight())*0.5f-5,cx=r.getCentreX(),cy=r.getCentreY()-1;
        const bool silver=slider.getProperties().getWithDefault("silver",false);
        const auto bounds=juce::Rectangle<float>(cx-radius,cy-radius,radius*2,radius*2);
        for(int i=0;i<=20;++i){const auto a=start+(end-start)*i/20.f;
            const float inner=radius+3,outer=inner+(i%2==0 ? 3.1f : 1.6f);
            g.setColour(colour(i%2==0 ? 0xffaeafa6 : 0xff73766b));
            g.drawLine(cx+std::sin(a)*inner,cy-std::cos(a)*inner,cx+std::sin(a)*outer,cy-std::cos(a)*outer,0.75f);}
        juce::Path shadow;shadow.addEllipse(bounds.reduced(0.8f));
        juce::DropShadow(juce::Colours::black.withAlpha(0.85f),6,{1,5}).drawForPath(g,shadow);
        g.setGradientFill(metalLight(bounds.expanded(2),true));g.fillEllipse(bounds.expanded(2));
        g.setColour(colour(0xff080a07));g.fillEllipse(bounds.expanded(0.8f));
        // A slightly offset skirt makes the knob project out of the panel.
        auto skirt=bounds.translated(0,1.9f);
        g.setGradientFill(metalLight(skirt,silver));g.fillEllipse(skirt);
        g.setColour(juce::Colours::black.withAlpha(0.7f));g.drawEllipse(skirt,0.75f);
        const int flutes=silver ? 72 : 44;
        for(int i=0;i<flutes;++i){const float a=i*juce::MathConstants<float>::twoPi/static_cast<float>(flutes);
            const float highlight=0.18f+0.52f*std::max(0.f,std::cos(a+0.65f));
            g.setColour(juce::Colours::black.withAlpha(silver ? 0.26f : 0.76f));
            g.drawLine(cx+std::sin(a)*radius*0.78f,cy+1.9f-std::cos(a)*radius*0.78f,
                       cx+std::sin(a)*radius*0.99f,cy+1.9f-std::cos(a)*radius*0.99f,silver ? 0.5f : 1.8f);
            const float lit=a+0.019f;
            g.setColour(colour(silver ? 0xffeff1e8 : 0xff93998b).withAlpha(highlight));
            g.drawLine(cx+std::sin(lit)*radius*0.85f,cy+1.9f-std::cos(lit)*radius*0.85f,
                       cx+std::sin(lit)*radius*0.98f,cy+1.9f-std::cos(lit)*radius*0.98f,0.6f);}
        auto rim=bounds.reduced(radius*0.105f);
        g.setGradientFill(metalLight(rim,silver));g.fillEllipse(rim);
        auto face=rim.reduced(radius*0.085f);
        juce::ColourGradient dome(colour(silver ? 0xfff0f1e7 : 0xff52574e),cx-radius*0.36f,cy-radius*0.48f,
                                  colour(silver ? 0xff92978b : 0xff11140f),cx+radius*0.65f,cy+radius*0.8f,true);
        dome.addColour(0.42,colour(silver ? 0xffd0d4c8 : 0xff32372e));
        dome.addColour(0.78,colour(silver ? 0xffadb3a3 : 0xff1b2017));
        g.setGradientFill(dome);g.fillEllipse(face);
        g.setColour(colour(silver ? 0xff626c58 : 0xff090d06));g.drawEllipse(face,0.7f);
        // Concentric lathe marks are clipped to the silver face.
        if(silver)for(float inset=1.3f;inset<face.getWidth()*0.48f;inset+=1.15f){
            g.setColour(juce::Colours::white.withAlpha(0.09f));g.drawEllipse(face.reduced(inset),0.35f);}
        juce::Path glint;glint.addCentredArc(cx,cy,rim.getWidth()*0.5f-0.6f,rim.getHeight()*0.5f-0.6f,0,-1.45f,0.65f,true);
        g.setColour(juce::Colours::white.withAlpha(silver ? 0.65f : 0.28f));g.strokePath(glint,juce::PathStrokeType(0.8f));
        const auto angle=start+position*(end-start);
        // Raised cream index with bevels and an actual cast shadow.
        juce::Path pointer;const float tip=-radius*0.77f,base=-radius*0.23f;
        pointer.startNewSubPath(-radius*0.075f,base);pointer.lineTo(-radius*0.045f,tip);
        pointer.quadraticTo(0,tip-0.6f,radius*0.045f,tip);pointer.lineTo(radius*0.075f,base);pointer.closeSubPath();
        pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(cx,cy));
        juce::DropShadow(juce::Colours::black.withAlpha(0.8f),2,{1,1}).drawForPath(g,pointer);
        g.setGradientFill(juce::ColourGradient(colour(silver ? 0xff292f23 : 0xfffff9dd),cx-radius*0.2f,cy-radius,
                                             colour(silver ? 0xff676f5e : 0xffaaa88e),cx+radius*0.2f,cy,false));g.fillPath(pointer);
        g.setColour(colour(silver ? 0xff1d2517 : 0xffe9e4cb));g.strokePath(pointer,juce::PathStrokeType(0.45f));
        if(slider.hasKeyboardFocus(true)){g.setColour(amber.withAlpha(0.85f));g.drawEllipse(bounds.expanded(3.4f),0.8f);}
    }
    void drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour&,bool over,bool down)override {
        const auto r=b.getLocalBounds().toFloat().reduced(1);
        g.setGradientFill(metalLight(r,true));g.fillRoundedRectangle(r,3);
        auto inner=r.reduced(1.1f);
        juce::ColourGradient cap(colour(down ? 0xff22251e : 0xff63665b),inner.getX(),inner.getY(),colour(0xff24271f),inner.getX(),inner.getBottom(),false);
        cap.addColour(0.18,colour(down ? 0xff30352a : 0xff42473b));g.setGradientFill(cap);g.fillRoundedRectangle(inner,2);
        g.setColour(juce::Colours::white.withAlpha(over ? 0.28f : 0.12f));g.drawRoundedRectangle(inner.reduced(0.5f),1.5f,0.6f);
    }
};
class Key final:public juce::Button {
public:
    Key(juce::String label,bool toggle=true):Button(label){setClickingTogglesState(toggle);setWantsKeyboardFocus(true);setTooltip(label);}
    bool red=false,dark=false,compact=false;
    void paintButton(juce::Graphics& g,bool over,bool down)override {
        const auto r=getLocalBounds().toFloat();const bool active=getToggleState();
        if(compact){
            const auto frame=r.reduced(0.65f);
            g.setGradientFill(metalLight(frame,true));g.fillRoundedRectangle(frame,1.4f);
            auto lens=frame.reduced(1.3f);g.setColour(colour(0xff070a05));g.fillRoundedRectangle(lens,0.8f);
            juce::ColourGradient glass(colour(active ? 0xff555c36 : 0xff4e5147),lens.getX(),lens.getY(),
                                       colour(active ? 0xff272d18 : 0xff171b12),lens.getX(),lens.getBottom(),false);
            glass.addColour(0.2,colour(active ? 0xff313b1c : 0xff2b3023));g.setGradientFill(glass);g.fillRoundedRectangle(lens.reduced(0.5f),0.6f);
            if(active){juce::ColourGradient light(amber.withAlpha(0.14f),lens.getCentreX(),lens.getCentreY(),juce::Colours::transparentBlack,lens.getRight(),lens.getY(),true);g.setGradientFill(light);g.fillRect(lens.reduced(1));}
            g.setColour(juce::Colours::white.withAlpha(over ? 0.24f : 0.08f));g.drawLine(lens.getX()+1,lens.getY()+1,lens.getRight()-1,lens.getY()+1,0.6f);
            hardware::text(g,getButtonText(),r.translated(0,0.8f),11,juce::Colours::black.withAlpha(0.8f));
            hardware::text(g,getButtonText(),r,11,active ? colour(0xffffce65) : muted);
            if(hasKeyboardFocus(true)){g.setColour(amber);g.drawRoundedRectangle(frame,1.4f,1);}
            return;
        }
        const float cx=r.getCentreX();
        hardware::text(g,getButtonText(),{0,0,r.getWidth(),12},9,dark?muted:colour(0xff484746),juce::Justification::centred,true);
        led(g,cx,18,active,red?colour(0xfff05a34):colour(0xffb6d46d),2.5f);
        keycap(g,r.withTrimmedTop(24).reduced(3.5f,0).withHeight(21),down||active,red,over);
        if(hasKeyboardFocus(true)){g.setColour(amber);g.drawRoundedRectangle(r.withTrimmedTop(23).reduced(1.5f,0).withHeight(24),2,1);}
    }
};
}
