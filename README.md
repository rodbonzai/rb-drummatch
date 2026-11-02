# KitMatch (v1.0.0-beta.1)

> **Intelligent Acoustic Complementarity Drumkit Matching & Auditioning System**  
> Diseñado con estética de hardware retrofuturista inspirada en la legendaria **AKAI MPC 60**.

---

## 🥁 Visión general

**KitMatch** es una aplicación de escritorio *local-first* creada con **JUCE y C++20** que ayuda a productores musicales y beatmakers a ensamblar kits de batería coherentes y complementarios a partir de múltiples librerías independientes de samples.

En lugar de limitarse a buscar sonidos "parecidos", KitMatch analiza las propiedades acústicas del sample de referencia (brillo espectral, decaimiento temporal, energía RMS y balance frecuencial) para encontrar y sugerir automáticamente las mejores combinaciones que se complementen musicalmente en un kit de 16 pads.

---

## ✨ Características principales

- **Pads 4x4 interactivos (Layout MPC Live 2):**
  - Mapeo directo con el teclado del computador:
    - **Fila 1 (A01 - A04):** `Z`, `X`, `C`, `V` *(Kick, Snare, Closed Hat, Open Hat)*
    - **Fila 2 (A05 - A08):** `A`, `S`, `D`, `F` *(Ghost Kick, Ghost Snare, Cross Stick, Closed Hat Hard)*
    - **Fila 3 (A09 - A12):** `Q`, `W`, `E`, `R` *(Crash Cymbal, Ride, Perc 01, Perc 02)*
    - **Fila 4 (A13 - A16):** `1`, `2`, `3`, `4` *(Tom Low, Tom Mid, Tom High, Rim)*
    - Compatible también con el teclado numérico (**Numpad**).
- **Monofonía por Pad (Self-Choke / Voice Retrigger):**
  - Cada pad cuenta con su propia voz dedicada de audio. Tocar el mismo pad repetidas veces reinicia el sample desde el principio de inmediato, eliminando acumulación de volumen, duplicaciones o solapamiento indeseado en samples largos.
  - Polifonía completa e independiente entre pads distintos.
- **Motor de Inferencia y Clasificación Inteligente:**
  - Reconocimiento avanzado multi-idioma y por tokens/abreviaturas estándar de la industria (`bombo`, `caja`, `kc`, `sd`, `sn`, `hh`, `oh`, `cr`, `rd`, `cl`, `clp`, `rm`, `stick`, etc.).
  - Jerarquía estricta: el nombre del archivo es la máxima autoridad para evitar falsos positivos provocados por carpetas contenedor genericamente nombradas.
- **Algoritmo de Matching Acústico:**
  - Botones **MATCH KIT**, **AUTO KIT** y **VARY** para generar combinaciones completas de 16 pads sin muestras duplicadas.
  - Badges con porcentaje de compatibilidad acústica en tiempo real en cada pad.
- **Protección del Sample de Referencia:**
  - El sonido de referencia activo queda bloqueado y protegido contra sobreescrituras automáticas; solo se sustituye si el usuario limpia manualmente ese pad específico o vacía el kit completo (**CLEAR ALL**).
- **Transporte y Visualización de Forma de Onda:**
  - Barra inferior con waveform interactiva, visualización de playhead en tiempo real, metadatos de audio (sample rate, bit depth, canales) y knob vintage de volumen master.
- **Base de Datos Local (SQLite):**
  - Indexación ultrarrápida y persistencia de librerías, kits y configuraciones.

---

## 🛠️ Requisitos y Compilación

### Requisitos:
- **macOS** 11.0 o superior (Apple Silicon M1/M2/M3/M4 o Intel)
- **CMake** 3.22 o superior
- Compilador con soporte para **C++20** (Apple Clang / Xcode Command Line Tools)

### Compilación:
```bash
# 1. Clonar el repositorio
git clone https://github.com/rodbonzai/rb-drummatch.git
cd rb-drummatch

# 2. Configurar con CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 3. Compilar la aplicación
cmake --build build --config Release -j4

# 4. Ejecutar
open build/DrumMatch_artefacts/Release/KitMatch.app
```

---

## ⌨️ Atajos de teclado

| Tecla | Acción |
| :--- | :--- |
| `Espacio` | Reproducir / Detener preview del sample en barra de transporte |
| `Z`, `X`, `C`, `V` | Disparar Pads A01 - A04 |
| `A`, `S`, `D`, `F` | Disparar Pads A05 - A08 |
| `Q`, `W`, `E`, `R` | Disparar Pads A09 - A12 |
| `1`, `2`, `3`, `4` | Disparar Pads A13 - A16 |

---

## 🏷️ Control de versiones

- **Versión actual:** `v1.0.0-beta.1`
- **Branding:** KitMatch (RB-KitMatch)

---

## 📄 Licencia

Desarrollado por RodBonzai. Todos los derechos reservados.
