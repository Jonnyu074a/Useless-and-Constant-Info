# Ronda 3: integración con Lazer UI (kamol1dn.lazer-ui)

Se revisó el código fuente de Lazer UI. **No se copió ni se fusionó nada de su código** (el repo no tiene licencia, sus sonidos son CC-BY-NC y su README dice que nunca irá al índice de Geode). La integración es solo de compatibilidad, usando su propio mecanismo público:

- Lazer UI recoge un frame después de `MenuLayer::init` los botones que otros mods dejan en `bottom-menu`, `right-side-menu`, `side-menu`, `top-right-menu` y `profile-menu`, copia su sprite y usa el ID del nodo como tooltip. Nuestro botón ya cumplía eso, pero:
  - usaba el sprite de estadísticas → **icono propio** (`resources/hud-icon.png`, glifo blanco plano; botón redondo cuando Lazer UI no está).
  - ID `uaci-hud-button` → ahora `info-overlay-button`_spr (tooltip "info overlay").
- Las pantallas de Lazer UI son nodos dentro de `MenuLayer` con ID `kamol1dn.lazer-ui/<nombre>`; `detectScreenName()` los reconoce (solo si ese mod está cargado).

**Por verificar en el dispositivo:** que los overlays cerrados de Lazer UI estén realmente con `visible=false` (si no, la etiqueta podría seguir mostrando su nombre tras cerrarlos), y que `resources/hud-icon.png` se empaquete (`"hud-icon.png"_spr`).

---

# Ronda 2 (bugs reportados en el dispositivo)

- **Crash al volver al juego** (`AppDelegate::pauseSound()`, SIGILL): eliminado el hook de `AppDelegate`. El tiempo de juego se guarda cada 30 s desde el overlay.
- **Texto bajo el icono HUD**: eliminado.
- **Label de pantalla**: ahora se detecta en vivo (popup superior de cualquier mod, p. ej. MegaHack, o la capa principal de la escena) con `geode::cocos::getObjectName`. Se quitaron los hooks por capa.
- **Colores sobre el texto**: ahora son filas con el swatch como botón.
- **Scroll que subía al elegir una opción**: se conserva la posición (solo al cambiar de pestaña va arriba).
- **Página del mod**: añadidos `about.md`, `changelog.md` y `logo.png` (Geode no lee el campo `changelog` de `mod.json`; se quitó). **Sustituye los de la plantilla en tu repo.**
- **Ventana rediseñada**: filas en paneles, iconos en pestañas, botón "i" con popup explicativo en cada opción y secciones, referencia de tags en el Builder, sección About.

---

# v5.2.0 — correcciones

**Estructura:** `include/` · `src/main.cpp` · `src/hooks/MenuLayer.cpp` · `src/ui/SettingsLayer.*` · `src/utils/Helpers.*`
(coincide con los globs del `CMakeLists.txt`).

## Críticos
1. Ventana de ajustes **rehecha sobre `geode::Popup`** (Geode 5, sin template), 400×290: cabe en los 320 de alto de GD. Ya no deja pasar toques al menú de atrás.
2. **Guardado seguro**: `OverlayConfig::save()` escribe las 37 claves con el listener suprimido (`g_suppressReload`). Los cambios se aplican y guardan al instante (ya no "solo al cerrar").
3. Offsets (`offset-x/y`) ahora **sí se guardan**; rango ampliado a ±2000 en `mod.json`.
4. Arrastre: conserva la esquina elegida, usa el rectángulo real del overlay, botón "Done" propio (capa en `OverlayManager`).
5. Color picker: `ColorPickPopup` de Geode (se cierra, sin firma de callback incorrecta).
6. CPS: hook en `GJBaseGameLayer::handleButton` y **botón 1 = salto** (el original comparaba con 0 → CPS siempre 0).
7. `m_attempts`, `m_normalPercent`, `m_levelID`, `m_playerUserID` leídos con `valInt()` (soporta `.value()` o int plano).

## Lógica
- Hide on enter / leave level: semántica corregida (siempre vuelve a mostrarse si la opción está apagada).
- Botón HUD dentro de `right-side-menu` (junto al Cofre Diario); fallback seguro si falta el sprite.
- TPS real vía `GJBaseGameLayer::processCommands`; FPS/TPS medidos en tiempo real (no `dt` escalado).
- Precisión: cuenta frames con colisión mortal (válido con noclip); "solo con noclip" = hubo colisión sin morir.
- Muertes: solo cuando `destroyPlayer` realmente mata; son de toda la sesión de juego.
- Sesión: `g_sessionStart` ya no se reinicia al perder el foco; el tiempo total se guarda cada 60 s.
- Menús de mods: solo mods instalados (sin red, sin falsos positivos). Se eliminó el uso de `WebTask`.
- Nombre de pantalla: sin nombres obsoletos (`"Menu"` si ninguna capa conocida se anunció).
- Formato: motor de una pasada (`renderFormat`): divide por `separator`, descarta segmentos vacíos; nombres con `{}` o `" | "` ya no rompen nada. El Builder usa el mismo motor y guarda el separador elegido.
- `{fps}/{tps}` con "show together" ya no deja una barra sobrante; el preset Developer usa `{tps}` aparte.
- Menús con tamaño explícito (los `CCMenu::create()` sin tamaño medían como la pantalla entera).
- Tag grid del Builder: 140 de alto (antes 110, se desbordaba). Sin emojis en fuentes bitmap.
- Cambios de pestaña diferidos un frame (no se destruye el botón que se está pulsando).
- `setString` solo cuando el texto cambió; recuento de mods re-escaneado cada 5 s; `-march=armv8-a` eliminado.

## Eliminado a propósito
- Soporte de macOS e iOS (solo Windows + Android).
- Hooks de `PauseLayer` ("Paused" nunca se mostraba) y `ProfilePage` (es un popup, dejaba el nombre pegado).

## ⚠️ No compilado — verificar al compilar
No tenía el Geode SDK, así que no pude compilar. Puntos con más riesgo de firma distinta en tus bindings:
- `GJBaseGameLayer::processCommands(float)` — si no existe, borra ese hook (TPS cae a FPS solo).
- `ColorPickPopup::create(ccColor3B)` / `setCallback(...)`.
- `TextInput::setString(str, false)` / `setMaxCharCount`.
- `ScrollLayer::moveToTop()`, `Popup::setTitle(...)`, `Popup::keyBackClicked()` (protegido, se usa desde la propia clase).
- `Mod::isLoaded()` (renombrado de `isEnabled` en v5) para el conteo de mods activos.
- `AppDelegate::applicationWillResignActive` (se dejó igual que el original).
