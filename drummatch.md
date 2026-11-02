# DrumMatch --- Documento de continuidad para Antigravity

**Versión del documento:** 1.0\
**Fecha:** 1 de noviembre de 2026\
**Plataforma inicial:** macOS / Apple Silicon (MacBook Air M1)\
**Tecnología definida:** JUCE + C++20 + CMake\
**Tipo de aplicación:** Standalone de escritorio (no plugin VST/AU en
esta primera etapa)

------------------------------------------------------------------------

## 1. Visión del proyecto

DrumMatch será una aplicación de escritorio para explorar, comparar y
seleccionar samples de cuatro o más bibliotecas de Drumkits, buscando
combinaciones que se complementen sonoramente.

El objetivo no es simplemente encontrar sonidos parecidos. La aplicación
debe ayudar a descubrir combinaciones musicalmente útiles, por ejemplo:

-   Un Kick de una biblioteca.
-   Una Snare de otra biblioteca.
-   Un Closed Hat de una tercera.
-   Percusiones de una cuarta.

El usuario selecciona manualmente un sonido de referencia. DrumMatch
analiza ese sample y busca en las carpetas habilitadas otros sonidos que
puedan complementarlo. Los resultados se presentan en un grid de pads
inspirado en la distribución visual de la MPC Live 2.

La aplicación debe tener identidad visual propia. No debe copiar
literalmente Maschine, FabFilter ni Ableton.

### Principios fundamentales

1.  **Exploración manual:** el usuario decide qué sonido utilizar como
    referencia y qué candidatos conservar.
2.  **Compatibilidad sonora:** el algoritmo debe evaluar
    complementariedad, no solamente similitud.
3.  **Grid estable:** cada pad tiene una posición y una categoría
    configurables.
4.  **Audición antes de decidir:** los samples y combinaciones deben
    poder escucharse.
5.  **No destructivo:** nunca modificar los samples originales.
6.  **Local-first:** el análisis y la biblioteca deben funcionar
    localmente, sin depender de servicios externos.
7.  **Exportación WAV:** exportar copias de los samples seleccionados,
    organizadas por categoría.

------------------------------------------------------------------------

## 2. Plataforma y tecnología

### Stack acordado

-   **C++20**
-   **JUCE Framework**
-   **CMake**
-   **Xcode / Apple Clang**
-   **SQLite** para persistencia del catálogo, layouts y kits.
-   Clases JUCE para audio, interfaz y DSP.
-   `juce::dsp::FFT` para análisis espectral inicial.
-   `juce::AudioFormatManager`, `juce::AudioBuffer` y componentes de
    audio JUCE para lectura y reproducción.

La primera compilación estará dirigida a macOS ARM64 (Apple Silicon /
M1). Mantener la arquitectura preparada para una eventual compilación
Intel o Windows, evitando dependencias innecesarias específicas de
Apple.

### Decisión importante

No utilizar Python como motor de la aplicación. El producto debe
desarrollarse en C++ con JUCE. Se pueden utilizar herramientas externas
durante el desarrollo, pero el funcionamiento final de DrumMatch debe
ser nativo.

------------------------------------------------------------------------

## 3. Concepto de interfaz

La referencia visual entregada por el usuario es una imagen de pads de
la MPC Live 2:

-   Fondo gris carbón / negro.
-   Pads cuadrados oscuros.
-   Bordes de colores según tipo de instrumento.
-   Identificadores de pad visibles (A01, A02, etc.).
-   Etiquetas de instrumento centradas dentro de cada pad.
-   Distribución de 4 columnas por 4 filas.
-   Estética funcional de hardware musical.

### Dirección visual

Desarrollar una interfaz inspirada en la experiencia visual de los pads
MPC, pero con marca y diseño propios:

-   Fondo grafito oscuro.
-   Pads rectangulares/cuadrados con bordes finos.
-   Tipografía técnica, clara y legible.
-   Colores por familia de instrumento.
-   Sin brillos, neones ni efectos decorativos exagerados.
-   La puntuación de compatibilidad debe ser secundaria al código de
    color del instrumento.
-   Los resultados deben mostrar nombre del sample y, cuando sea
    posible, un indicador de compatibilidad.
-   Incluir waveform y controles de escucha.

No continuar con las propuestas anteriores de estilo industrial, minimal
premium o futurista como diseños independientes. La referencia MPC
proporcionada por el usuario es la guía visual principal.

------------------------------------------------------------------------

## 4. Flujo principal de uso

1.  El usuario agrega las carpetas de Drumkits desde un menú lateral.
2.  Puede habilitar o deshabilitar cada carpeta para incluirla o
    excluirla de las búsquedas.
3.  DrumMatch escanea recursivamente las carpetas habilitadas.
4.  Se catalogan los archivos de audio compatibles.
5.  El usuario navega por los samples y selecciona uno como **Reference
    Sample**.
6.  El motor analiza el sample de referencia.
7.  El motor busca candidatos compatibles en todas las bibliotecas
    habilitadas.
8.  Los resultados se distribuyen en el grid según posiciones y
    categorías configuradas.
9.  El usuario escucha los candidatos, navega por alternativas y
    selecciona los que desea conservar.
10. El usuario guarda el kit y exporta copias WAV organizadas en
    carpetas.

### Conceptos que deben permanecer separados

-   **Reference Sample:** sample de referencia que inicia la búsqueda.
-   **Match Grid:** resultados encontrados por el algoritmo en las
    posiciones fijas del grid.
-   **Selected Kit:** selección definitiva del usuario. No debe cambiar
    automáticamente cuando se actualizan los resultados.

------------------------------------------------------------------------

## 5. Grid de 16 pads

El grid tendrá 16 posiciones permanentes por defecto, con posibilidad de
configurar la categoría y etiqueta de cada posición.

La posición identifica la función/categoría del pad; el sample que
contiene puede cambiar según los resultados.

### Layout inicial propuesto

La distribución visual sigue la numeración de la referencia MPC, de
arriba hacia abajo:

  --------------------------------------------------------------------------
  Fila           Columna 1      Columna 2      Columna 3      Columna 4
  -------------- -------------- -------------- -------------- --------------
  1              A13 --- TOM    A14 --- TOM    A15 --- TOM    A16 --- RIM
                 LOW            MID            HIGH           

  2              A09 --- CRASH  A10 --- RIDE   A11 --- PERC   A12 --- PERC
                 CYMBAL                        01             02

  3              A05 --- GHOST  A06 --- GHOST  A07 --- CROSS  A08 --- CLOSED
                 KICK           SNARE          STICK          HAT HARD

  4              A01 --- KICK   A02 --- SNARE  A03 --- CLOSED A04 --- OPEN
                                               HAT            HAT
  --------------------------------------------------------------------------

### Colores iniciales por categoría

  Categoría                                 Color sugerido
  ----------------------------------------- ----------------
  Kick / Ghost Kick                         Rojo
  Snare / Ghost Snare / Cross Stick / Rim   Azul
  Closed Hat / Open Hat / Hard Hat          Verde
  Crash / Ride / Toms                       Amarillo
  Percussions                               Morado
  Otros                                     Gris neutro

Los colores deben ser configurables, pero debe existir una paleta
inicial coherente.

### Configuración de posiciones

El usuario confirmó que quiere posiciones configurables. Debe existir
una pantalla o modo de configuración que permita:

-   Cambiar la categoría asignada a un pad.
-   Cambiar su etiqueta.
-   Cambiar su color.
-   Mantener su identificador (A01--A16) o permitir renombrarlo si se
    considera conveniente.
-   Guardar varios layouts.
-   Restaurar el layout predeterminado.

El layout no debe depender de posiciones codificadas de forma rígida en
el componente visual. Debe provenir de un modelo de configuración
persistible.

### Comportamiento de resultados

-   Las posiciones no deben moverse cuando cambian los resultados.
-   Los candidatos se ordenan por compatibilidad dentro de su categoría.
-   El mejor candidato de cada categoría puede mostrarse inicialmente en
    su pad.
-   Si no existe candidato para una categoría, el pad queda vacío o
    indica "No match".
-   La puntuación no debe reemplazar el color de la categoría.
-   Al seleccionar un pad se debe poder escuchar el sample y consultar
    sus detalles.

------------------------------------------------------------------------

## 6. Navegación de alternativas

El usuario confirmó que quiere **ambas modalidades**:

### A. Navegación anterior / siguiente

Al seleccionar un pad, el usuario puede recorrer los candidatos
compatibles de esa categoría:

-   Previous.
-   Next.
-   Número de alternativa actual / total.
-   Puntuación de compatibilidad.
-   Nombre y biblioteca de origen.

Cambiar de alternativa no debe modificar los otros pads ni el Reference
Sample.

### B. Lista completa de alternativas

Al seleccionar un pad, debe poder abrirse un panel/lista con todos los
candidatos compatibles de esa categoría, ordenados por ranking.

La lista debe mostrar, cuando esté disponible:

-   Nombre del archivo.
-   Biblioteca / carpeta de origen.
-   Duración.
-   Sample rate.
-   Puntuación de compatibilidad.
-   Botón o acción para previsualizar.
-   Acción para asignar el candidato al pad.

La navegación anterior/siguiente y la lista completa deben operar sobre
la misma colección ordenada de candidatos.

------------------------------------------------------------------------

## 7. Organización de la ventana principal

Propuesta de zonas:

### Sidebar izquierdo --- Library

-   Lista de carpetas agregadas.
-   Nombre de cada biblioteca.
-   Checkbox o toggle para incluir/excluir en búsquedas.
-   Agregar carpeta.
-   Quitar carpeta de DrumMatch (sin borrar archivos originales).
-   Escanear / actualizar biblioteca.
-   Estado del escaneo y cantidad de samples.

### Zona central --- Pad Grid

-   Grid de 4 × 4.
-   16 posiciones configurables.
-   Identificador de pad.
-   Categoría.
-   Nombre del sample asignado o estado vacío.
-   Código de color por instrumento.
-   Indicador de compatibilidad.
-   Estado de selección.

### Panel de detalles / alternativas

Al seleccionar un sample o pad:

-   Nombre completo del archivo.
-   Ruta y biblioteca de origen.
-   Categoría detectada o asignada.
-   Duración y sample rate.
-   Waveform.
-   Características acústicas.
-   Puntuación y explicación del match.
-   Navegación anterior/siguiente.
-   Lista completa de candidatos.

### Barra inferior de audio

-   Play / Stop.
-   Volumen.
-   Posición de reproducción.
-   Waveform o progreso.
-   Más adelante, escucha conjunta del kit.

La reproducción debe implementarse de forma segura para audio en tiempo
real. No hacer lectura de archivos, asignaciones dinámicas ni cálculos
pesados dentro del callback de audio.

------------------------------------------------------------------------

## 8. Motor de biblioteca

### Primera etapa de formatos

Soporte inicial:

-   WAV.
-   AIFF / AIF.

La arquitectura debe permitir agregar otros formatos compatibles con
JUCE posteriormente.

### Escaneo

-   Escaneo recursivo de subcarpetas.
-   No modificar ni mover archivos originales.
-   Permitir habilitar/deshabilitar bibliotecas.
-   Guardar ruta absoluta y ruta relativa.
-   Detectar archivos duplicados mediante hash o estrategia equivalente.
-   Evitar volver a analizar archivos que no hayan cambiado.
-   Detectar archivos eliminados o movidos y actualizar el catálogo.
-   Mostrar errores de lectura sin detener todo el escaneo.

### Datos mínimos por sample

-   ID interno estable.
-   Ruta absoluta.
-   Ruta relativa.
-   Nombre de archivo.
-   Nombre de biblioteca.
-   Extensión.
-   Tamaño en bytes.
-   Fecha de modificación.
-   Duración.
-   Sample rate.
-   Número de canales.
-   Profundidad/formato cuando pueda obtenerse.
-   Hash para detectar duplicados.
-   Categoría asignada o detectada.
-   Estado de análisis.

La categoría puede inferirse inicialmente por nombre/ruta, pero el
usuario debe poder corregirla manualmente. No asumir que todos los
nombres siguen convenciones consistentes.

------------------------------------------------------------------------

## 9. Motor de análisis acústico

El análisis debe ejecutarse en segundo plano y guardar resultados para
reutilizarlos.

### Características iniciales

-   RMS / energía promedio.
-   Peak.
-   Crest Factor.
-   Spectral Centroid (brillo).
-   Spectral Rolloff.
-   MFCC o descriptor tímbrico equivalente.
-   Attack Time.
-   Decay / duración.
-   Energía por bandas de frecuencia.
-   Distribución espectral.
-   Características de transientes.

Usar `juce::dsp::FFT` y operaciones C++ eficientes. Considerar ventanas,
tamaño de FFT y análisis de envolvente apropiados para samples
percusivos.

### Compatibilidad: criterio esencial

No confundir similitud con complementariedad.

Ejemplo: - Un Kick profundo puede combinar bien con una Snare
brillante. - Dos sonidos con mucha energía en la misma zona pueden
competir. - Una Snare distinta al Kick no debe recibir automáticamente
una puntuación baja por ser diferente.

La compatibilidad debe considerar el tipo de relación entre
instrumentos:

  Pareja               Aspectos a considerar
  -------------------- -------------------------------------------------------
  Kick + Snare         Conflicto en graves/medios, ataque, cuerpo y densidad
  Kick + Closed Hat    Separación de graves y agudos, transientes
  Kick + Open Hat      Balance espectral y duración
  Snare + Hi-Hat       Presencia, brillo, densidad de transientes
  Kick + Percussion    Solapamiento espectral y temporal
  Snare + Percussion   Enmascaramiento y complementariedad tímbrica
  Kit completo         Balance global y coherencia

### Puntuación

-   La primera versión puede utilizar reglas ponderadas y normalizadas.
-   Los pesos deben poder ajustarse.
-   No presentar porcentajes ficticios como si fueran una verdad
    musical.
-   Explicar por qué una combinación recibe una puntuación.
-   La audición humana sigue siendo el criterio final.
-   No afirmar que una puntuación matemática garantiza que el kit suene
    bien en una mezcla.

Diseñar el motor para poder mejorar el algoritmo más adelante, por
ejemplo incorporando embeddings de audio o modelos especializados, sin
cambiar la UI ni la base de datos.

------------------------------------------------------------------------

## 10. Audio Engine

Responsabilidades:

-   Cargar WAV/AIFF mediante JUCE.
-   Previsualizar un sample individual.
-   Detener/reemplazar reproducción de forma segura.
-   Mostrar waveform.
-   Ajustar volumen de escucha.
-   Más adelante, reproducir varios samples para escuchar el kit.

Usar una arquitectura basada en `AudioAppComponent` o un diseño
equivalente con `AudioDeviceManager` y fuentes de audio JUCE.

Reglas de audio en tiempo real:

-   No leer archivos desde el callback.
-   No usar mutex bloqueante en el callback.
-   No reservar memoria dinámicamente en el callback.
-   Preparar buffers y recursos antes de reproducir.
-   La UI debe comunicarse con el motor mediante mecanismos seguros.
-   La reproducción de previsualización debe ser independiente del
    análisis acústico.

------------------------------------------------------------------------

## 11. Kit Builder y exportación

### Kit Builder

Debe mantener una selección independiente de los resultados:

-   El usuario puede aceptar un candidato de un pad.
-   Puede reemplazarlo por otra alternativa.
-   Puede dejar un pad vacío.
-   Puede guardar varios kits.
-   Cambiar el Reference Sample no debe borrar la selección guardada.
-   El kit debe conservar referencia a los samples originales.

### Exportación WAV

La primera versión exportará copias de los samples originales, sin
normalización, procesamiento, conversión ni modificación automática.

Estructura de salida propuesta:

``` text
DrumMatch/
└── Exports/
    └── Kit_Dark_HipHop_001/
        ├── Kicks/
        ├── Snares/
        ├── HiHats/
        ├── Cymbals/
        ├── Percussions/
        ├── Toms/
        └── kit_info.txt
```

El exportador debe:

-   Crear las carpetas según categorías.
-   Copiar los WAV originales.
-   Evitar sobrescribir archivos sin confirmación.
-   Resolver colisiones de nombres de forma segura.
-   Generar un resumen con nombre, categoría, origen y pad asignado.
-   Informar errores de copia.
-   No borrar ni modificar las bibliotecas originales.

------------------------------------------------------------------------

## 12. Persistencia SQLite

SQLite almacenará:

-   Bibliotecas registradas y su estado activo.
-   Samples indexados.
-   Metadatos de audio.
-   Características acústicas.
-   Categorías.
-   Configuraciones de pads.
-   Layouts.
-   Kits guardados.
-   Samples asignados a cada kit.

Separar el acceso a datos del código de interfaz. Usar
migraciones/versionado de esquema para poder actualizar la base de datos
sin perder la biblioteca.

------------------------------------------------------------------------

## 13. Arquitectura de carpetas C++ propuesta

``` text
DrumMatch/
├── CMakeLists.txt
├── README.md
├── JUCE/                       # Submódulo o checkout local de JUCE
├── Source/
│   ├── Main.cpp
│   ├── App/
│   │   ├── AppController.h
│   │   └── AppController.cpp
│   ├── UI/
│   │   ├── MainComponent.h
│   │   ├── MainComponent.cpp
│   │   ├── SidebarComponent.h
│   │   ├── SidebarComponent.cpp
│   │   ├── PadGridComponent.h
│   │   ├── PadGridComponent.cpp
│   │   ├── PadComponent.h
│   │   ├── PadComponent.cpp
│   │   ├── AlternativeBrowser.h
│   │   ├── AlternativeBrowser.cpp
│   │   ├── SampleDetailComponent.h
│   │   ├── SampleDetailComponent.cpp
│   │   └── AudioPlayerComponent.h
│   │       └── AudioPlayerComponent.cpp
│   ├── Domain/
│   │   ├── Sample.h
│   │   ├── InstrumentCategory.h
│   │   ├── PadDefinition.h
│   │   ├── CompatibilityResult.h
│   │   └── DrumKit.h
│   ├── Library/
│   │   ├── LibraryScanner.h
│   │   ├── LibraryScanner.cpp
│   │   ├── SampleCatalog.h
│   │   └── SampleCatalog.cpp
│   ├── Audio/
│   │   ├── AudioEngine.h
│   │   ├── AudioEngine.cpp
│   │   ├── SamplePlayer.h
│   │   ├── SamplePlayer.cpp
│   │   ├── WaveformCache.h
│   │   └── WaveformCache.cpp
│   ├── Analysis/
│   │   ├── FeatureExtractor.h
│   │   ├── FeatureExtractor.cpp
│   │   ├── CompatibilityEngine.h
│   │   ├── CompatibilityEngine.cpp
│   │   ├── MatchRanking.h
│   │   └── MatchRanking.cpp
│   ├── Persistence/
│   │   ├── Database.h
│   │   ├── Database.cpp
│   │   └── Repositories/
│   ├── Kit/
│   │   ├── KitBuilder.h
│   │   └── KitBuilder.cpp
│   └── Export/
│       ├── KitExporter.h
│       └── KitExporter.cpp
├── Resources/
└── Tests/
```

La estructura puede ajustarse durante la implementación, pero mantener
separación entre UI, dominio, audio, análisis, persistencia y
exportación.

------------------------------------------------------------------------

## 14. Estado del starter ya generado

Existe un starter inicial llamado `DrumMatch_JUCE_Starter.zip`, generado
durante la conversación.

Incluye:

-   `CMakeLists.txt`.
-   `README.md`.
-   `Source/Main.cpp`.
-   `Source/MainComponent.h/.cpp`.
-   `Source/LibraryScanner.h/.cpp`.
-   `Source/PadGridComponent.h/.cpp`.

Implementación inicial:

-   Aplicación Standalone JUCE.
-   Ventana oscura.
-   Sidebar con botón para agregar carpetas.
-   Escaneo recursivo de WAV y AIFF/AIF.
-   Grid visual de 16 pads con identificadores y colores.
-   Callback de selección de pad.
-   Catálogo temporal en memoria.

Limitaciones conocidas del starter:

-   No tiene todavía motor de reproducción funcional: el callback de
    audio solo limpia el buffer.
-   No tiene waveform.
-   No tiene SQLite ni persistencia.
-   No calcula características acústicas.
-   No calcula compatibilidad real.
-   No implementa navegación real de alternativas.
-   No exporta kits.
-   El layout del grid está codificado como valor predeterminado; debe
    migrarse a configuración persistible.
-   La lista de samples debe integrarse completamente en la UI y
    validarse al compilar.
-   El escaneo inicial es síncrono y debe migrarse a una tarea en
    segundo plano antes de usarlo con bibliotecas grandes.

**No asumir que el starter ya está completo ni que compila sin
ajustes.** Primero inspeccionar el código, compilarlo y corregir errores
antes de agregar funcionalidades.

------------------------------------------------------------------------

## 15. Plan de desarrollo recomendado

### Fase 1 --- Base de aplicación y Library

1.  Revisar y compilar el starter.
2.  Consolidar CMake y la versión de JUCE.
3.  Implementar el modelo de bibliotecas.
4.  Agregar/quitar carpetas.
5.  Habilitar/deshabilitar carpetas.
6.  Escaneo asíncrono recursivo.
7.  Listado de samples con filtros.
8.  Persistencia SQLite.

**Criterio de aceptación:** agregar cuatro carpetas, escanearlas sin
congelar la interfaz, activar/desactivar cada una y volver a abrir la
aplicación conservando el catálogo.

### Fase 2 --- Grid configurable

1.  Crear modelo `PadDefinition`.
2.  Cargar el layout predeterminado A01--A16.
3.  Permitir cambiar categoría, etiqueta y color.
4.  Guardar múltiples layouts en SQLite.
5.  Seleccionar pads y mostrar su detalle.

**Criterio de aceptación:** editar la asignación de categorías, guardar
el layout, reiniciar la aplicación y recuperar exactamente la
configuración.

### Fase 3 --- Reproducción y waveform

1.  Reproducir WAV/AIFF.
2.  Stop y reemplazo seguro de sample.
3.  Volumen de preescucha.
4.  Waveform.
5.  Navegación entre samples.

**Criterio de aceptación:** preescuchar samples de bibliotecas grandes
sin bloquear la interfaz ni generar fallos de audio.

### Fase 4 --- Análisis y compatibilidad

1.  Implementar extractor de características.
2.  Ejecutar análisis en segundo plano.
3.  Persistir características.
4.  Implementar reglas de complementariedad por pares.
5.  Crear ranking por categoría.
6.  Completar los pads con el mejor candidato disponible.
7.  Explicar puntuaciones y permitir ajustar pesos.

**Criterio de aceptación:** al seleccionar un Kick, DrumMatch busca
candidatos en las bibliotecas activas y completa las posiciones de
categorías compatibles, manteniendo las posiciones fijas.

### Fase 5 --- Alternativas y Kit Builder

1.  Anterior/siguiente.
2.  Lista completa de candidatos.
3.  Asignación manual al pad.
4.  Selección independiente del Match Grid.
5.  Guardar y recuperar kits.

**Criterio de aceptación:** cambiar una alternativa no modifica otros
pads ni el sample de referencia; guardar y recuperar un kit conserva las
asignaciones.

### Fase 6 --- Exportación

1.  Seleccionar carpeta de destino.
2.  Crear subcarpetas por categoría.
3.  Copiar samples originales.
4.  Generar `kit_info.txt`.
5.  Manejar colisiones y errores.

**Criterio de aceptación:** exportar un kit y verificar que los archivos
resultantes son copias idénticas de los originales.

------------------------------------------------------------------------

## 16. Instrucciones para Antigravity

Actúa como desarrollador senior especializado en C++, JUCE,
procesamiento digital de audio y diseño de aplicaciones de escritorio.

Debes continuar el desarrollo de DrumMatch respetando las decisiones
funcionales y visuales de este documento.

### Reglas de trabajo

1.  Antes de modificar, inspecciona todos los archivos existentes.
2.  No reemplaces el proyecto completo ni elimines código funcional sin
    justificarlo.
3.  Compila después de cada etapa importante.
4.  Corrige errores de compilación antes de avanzar.
5.  Mantén C++20 y CMake.
6.  No introduzcas Python como dependencia de runtime.
7.  No implementes porcentajes ficticios de compatibilidad.
8.  No bloquees el hilo de interfaz ni el hilo de audio con escaneos o
    análisis.
9.  No modifiques nunca los samples originales.
10. Mantén la interfaz inspirada en los pads de la MPC Live 2, con 16
    posiciones configurables.
11. Mantén separados Reference Sample, Match Grid y Selected Kit.
12. Persiste bibliotecas, layouts y kits.
13. Implementa pruebas para lógica de ranking, configuración y
    exportación cuando corresponda.
14. Explica brevemente qué archivos cambiaste, qué se implementó y cómo
    probarlo.

### Primera tarea concreta

Comienza por la **Fase 1: revisar, corregir y consolidar el starter
JUCE**.

-   Inspecciona el CMakeLists y todos los archivos de `Source`.
-   Verifica la API de JUCE utilizada.
-   Compila la aplicación.
-   Corrige errores reales.
-   Implementa un escaneo asíncrono de carpetas.
-   Completa la lista de samples y selección de Reference Sample.
-   No avances todavía al algoritmo de compatibilidad.
-   Conserva el diseño y el grid de 16 pads.

Al terminar, entrega un resumen de los cambios y los pasos exactos para
compilar y ejecutar en macOS.

------------------------------------------------------------------------

## 17. Definición de éxito del producto

DrumMatch estará cumpliendo su propósito cuando el usuario pueda:

1.  Registrar sus cuatro bibliotecas de Drumkits.
2.  Seleccionar cuáles participan en la búsqueda.
3.  Elegir cualquier sample como referencia.
4.  Obtener candidatos complementarios en un grid de 16 posiciones fijas
    y configurables.
5.  Escuchar cada resultado.
6.  Recorrer alternativas con anterior/siguiente o lista completa.
7.  Armar manualmente un kit.
8.  Guardarlo y exportarlo como carpetas WAV sin alterar los originales.

**La prioridad es una herramienta musical rápida, clara y confiable. La
puntuación de compatibilidad debe ayudar a escuchar y decidir, nunca
reemplazar el criterio auditivo del usuario.**
