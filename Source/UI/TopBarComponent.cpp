#include "TopBarComponent.h"
#include "../Assets/LogoSVG.h"

namespace drummatch {

TopBarComponent::TopBarComponent(SampleCatalog &cat) : catalog(cat) {
  catalog.addListener(this);

  // Cargar vector logo retrofuturista RB-KITMATCH
  logoDrawable = juce::Drawable::createFromSVGString(assets::cleanedLogoSvg);

  // matchButton.onClick = [this]()
  // {
  //     if (onMatchRequested)
  //         onMatchRequested();
  // };
  // addAndMakeVisible(matchButton);

  // clearButton.onClick = [this]()
  // {
  //     if (onClearRequested)
  //         onClearRequested();
  //     else
  //         catalog.clearReferenceSample();
  // };
  // addAndMakeVisible(clearButton);

  updateReferenceUI();
}

TopBarComponent::~TopBarComponent() { catalog.removeListener(this); }

void TopBarComponent::setReferencePadId(const juce::String &padId) {
  referencePadId = padId;
  repaint();
}

void TopBarComponent::referenceSampleChanged(const std::optional<Sample> &) {
  updateReferenceUI();
}

void TopBarComponent::updateReferenceUI() { repaint(); }

void TopBarComponent::paint(juce::Graphics &g) {
  // Fondo de Chasis AKAI MPC 60 (#BAB7AA)
  g.fillAll(theme::chassis);

  // Borde inferior de separación del chasis
  g.setColour(theme::textSecondary.withAlpha(0.35f));
  g.drawHorizontalLine(getHeight() - 1, 0.0f, static_cast<float>(getWidth()));

  // Logo retrofuturista RB-KITMATCH alineado exactamente sobre la columna
  // DRUMKITS
  if (logoDrawable != nullptr) {
    logoDrawable->drawWithin(
        g, logoArea.toFloat(),
        juce::RectanglePlacement(juce::RectanglePlacement::xLeft |
                                 juce::RectanglePlacement::yMid |
                                 juce::RectanglePlacement::onlyReduceInSize),
        1.0f);
  } else {
    g.setColour(theme::textMain);
    g.setFont(theme::getSerigraphyFont(14.0f));
    g.drawText("RB-KITMATCH", logoArea, juce::Justification::centredLeft,
               false);
  }

  // Pantalla LCD ancha verde central alineada con PAD GRID
  if (!lcdArea.isEmpty()) {
    theme::drawLCDBox(g, lcdArea.toFloat(), 2.0f);

    auto textArea = lcdArea.reduced(8, 2);
    g.setColour(theme::lcdText);
    g.setFont(theme::getMonoFont(11.5f));

    const auto ref = catalog.getReferenceSample();
    if (ref.has_value()) {
      const juce::String p1 = "REF " + ref->getFileName().toUpperCase();
      const juce::String p2 =
          referencePadId.isNotEmpty()
              ? getCategoryName(ref->category) + "  [" + referencePadId + "]"
              : getCategoryName(ref->category);
      const juce::String p3 = ref->libraryName;
      const juce::String p4 = ref->getFormattedDuration();

      // Dibujar partes con separadores finos
      const int totalW = textArea.getWidth();
      const int w1 = juce::roundToInt(totalW * 0.30f);
      const int w2 = juce::roundToInt(totalW * 0.22f);
      const int w3 = juce::roundToInt(totalW * 0.33f);
      const int w4 = totalW - w1 - w2 - w3;

      auto r1 = textArea.removeFromLeft(w1);
      g.drawText(p1, r1, juce::Justification::centredLeft, true);
      g.setColour(theme::lcdText.withAlpha(0.35f));
      g.drawVerticalLine(r1.getRight() - 4,
                         static_cast<float>(textArea.getY() + 3),
                         static_cast<float>(textArea.getBottom() - 3));

      auto r2 = textArea.removeFromLeft(w2);
      g.setColour(theme::lcdText);
      g.drawText(p2, r2.reduced(6, 0), juce::Justification::centredLeft, true);
      g.setColour(theme::lcdText.withAlpha(0.35f));
      g.drawVerticalLine(r2.getRight() - 4,
                         static_cast<float>(textArea.getY() + 3),
                         static_cast<float>(textArea.getBottom() - 3));

      auto r3 = textArea.removeFromLeft(w3);
      g.setColour(theme::lcdText);
      g.drawText(p3, r3.reduced(6, 0), juce::Justification::centredLeft, true);
      g.setColour(theme::lcdText.withAlpha(0.35f));
      g.drawVerticalLine(r3.getRight() - 4,
                         static_cast<float>(textArea.getY() + 3),
                         static_cast<float>(textArea.getBottom() - 3));

      g.setColour(theme::lcdText);
      g.drawText(p4, textArea.reduced(6, 0), juce::Justification::centredRight,
                 false);
    } else {
      g.setColour(theme::lcdPlaceholder);
      g.drawText("NO REFERENCE SAMPLE  |  SELECT A SAMPLE IN THE RIGHT PANEL "
                 "TO AUDITION & MATCH",
                 textArea, juce::Justification::centredLeft, true);
    }
  }
}

void TopBarComponent::resized() {
  // Dimensiones idénticas a las 3 columnas de MainComponent:
  // Margen 14px, Gap 14px
  // Columna Izquierda (DRUMKITS):  x = 14, width = 280
  // Columna Central   (PAD GRID):  x = 308, width = dynamic (termina en
  // getWidth() - 14 - 330 - 14) Columna Derecha   (SAMPLES):   x = getWidth() -
  // 14 - 330, width = 330
  const int margin = 14;
  const int gap = 14;
  const int leftColW = 280;
  const int rightColW = 330;

  // 1. Sección Logo (sobre la columna DRUMKITS)
  logoArea = juce::Rectangle<int>(margin, 12, leftColW, 40);

  // 2. Sección LCD (alineada exactamente con la columna central PAD GRID)
  const int lcdX = margin + leftColW + gap;
  const int lcdRight = getWidth() - margin - rightColW - gap;
  const int lcdW = juce::jmax(200, lcdRight - lcdX);
  lcdArea = juce::Rectangle<int>(lcdX, 14, lcdW, 36);

  // 3. Sección Botones (sobre la columna derecha SAMPLES)
  const int buttonsX = getWidth() - margin - rightColW;
  auto buttonsArea = juce::Rectangle<int>(buttonsX, 15, rightColW, 34);

  // matchButton.setBounds(buttonsArea.removeFromLeft(150).withHeight(34));
  // clearButton.setBounds(buttonsArea.removeFromRight(110).withHeight(34));
}

} // namespace drummatch
