#pragma once

#include "../Domain/Sample.h"
#include "../Library/SampleCatalog.h"
#include "IconButton.h"
#include "MPC60Theme.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace drummatch {

class TopBarComponent : public juce::Component, public SampleCatalog::Listener {
public:
  TopBarComponent(SampleCatalog &catalog);
  ~TopBarComponent() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

  void referenceSampleChanged(const std::optional<Sample> &ref) override;

  void setReferencePadId(const juce::String &padId);

  std::function<void()> onMatchRequested;
  std::function<void()> onClearRequested;

private:
  void updateReferenceUI();

  SampleCatalog &catalog;
  juce::String referencePadId;

  //   IconButton matchButton { "MATCH KIT", IconType::Play,
  //   theme::ButtonStyle::Primary }; IconButton clearButton { "CLEAR ALL",
  //   IconType::None, theme::ButtonStyle::Danger };

  std::unique_ptr<juce::Drawable> logoDrawable;
  juce::Rectangle<int> logoArea;
  juce::Rectangle<int> lcdArea;
};

} // namespace drummatch
