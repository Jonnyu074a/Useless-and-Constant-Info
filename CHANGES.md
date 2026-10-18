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
