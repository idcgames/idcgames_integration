# Login del Launcher y verificación opcional del backend

El login nativo existente no se sustituye. El EXE inicializa IDC, recibe UserData por la DLL/Pipe y puede usar esa respuesta con su propio backend. GamePanel es una opción de backend, no un requisito del SDK.

## Qué es cada valor

- `id_game` / `gameId`: ID operativo IDC. Puede ser distinto del ID de fila del panel y del `game_id` interno de GamePanel.
- `game_token`: identificador público del cliente; `gameSecret` es un alias histórico del protocolo. No autoriza al jugador ni firma pagos.
- `UserData.extraParams.tokenUserGameId`: credencial temporal del jugador, nunca imprimirla ni enviarla en URL.
- `payment_secret`: clave privada por juego para Admin → backend. No va al EXE ni sirve para login.
- `X-Internal-Key`: credencial interna de IDC para verificación web; no entregarla a un estudio ni usarla como clave de pagos.

## Launcher → EXE

Consultar `launcher-protocol.md` y `quickstart-csharp.md`. Mantener el Launcher abierto y logueado, inicializar la DLL con ID operativo/token público y PID, mantener callbacks vivos y llamar Poll en el hilo del juego. Start sin error no equivale a UserData válido. El campo tokenUserGameId es histórico: no deducir JWT ni MD5 por su nombre.

## EXE → backend → Auth (opcional)

El EXE transmite la credencial ya recibida a su backend por HTTPS. Para juegos habilitados con `client_session_protocol=opaque-v1`, el backend utiliza el contrato moderno de sesión emitida:

```http
POST https://auth.idcgames.com/api/launcher/get-access-data
Content-Type: application/json
```

```json
{"token":"CREDENTIAL_FROM_LIVE_PIPE","gameId":1614}
```

El backend fija su gameId en configuración, no acepta que el cliente seleccione otro juego. La URL se conserva por compatibilidad pero la implementación opaque-v1 valida el registro emitido, juego, padre, caducidad, revocación y acceso vigente. Los aliases históricos idctk/idcgameid se aceptan, pero no son necesarios. No fabricar hashes ni tratar la credencial como JWT web. No requiere una clave S2S global adicional: la credencial temporal limitada al juego es el bearer.

Exigir HTTP correcto y todos los campos de éxito: result0, can_playtrue, game_id esperado, user.userid positivo, expires_at futuro (segundos Unix) y catalog_syncedtrue antes de desbloquear contenido del catálogo. Crear sesión propia vinculada a (game_id,user.userid), ignorando IDs/nicks/emails propuestos por el cliente. No usar usercrc como firma.

Los enteros de dlcs son IDs operativos autorizados, no panel_id ni SKU ni catálogo completo. [] no desbloquea nada. Developer de base no regala DLC. Revalidar acciones protegidas y revocaciones; TTL máximo30min sin superar el padre/derecho. Renovar con el ciclo InitLib soportado, nunca alargar autorización offline.

Errores401 para credencial no emitida/caducada/revocada/otro juego;403 para mantenimiento/baneo/acceso denegado;503 para dependencia no disponible. Revisar error_code y mensaje, rechazar timeout/JSON incompleto sin fallback a permisos antiguos. No incluir tokens en trazas.

La aprobación/sincronización de desarrollo desde Admin registra primero el juego en Auth. Confirmar protocolo y entrega antes del test. Un probe negativo401 no demuestra login real; las fixtures no sustituyen probar el EXE con Launcher.

## Si se utiliza GamePanel

El ejemplo CSharp_Test_1599 obtiene UserData y después llama POST /api/v1/auth/third-party/login en GamePanel con platform idc, token, user_id, game_id interno de GamePanel, idc_game_id operativo y la configuración de integración de esa instalación. Ese login devuelve una sesión propia de GamePanel; /auth/validate comprueba su bearer.

El adaptador histórico prueba check-user-game-token con userId/gameId/tokenUserGameId y después verify-token para el formato web. Un check OK permite verificar el login, pero no devuelve la proyección completa de caducidad/DLC. Para opaque-v1, un fallo nativo no debe convertirse en intento de JWT web ni aceptación local. Contrastar la versión instalada del adaptador antes de integrar; no confundir su ID interno con el operativo.

## Compatibilidad web histórica

POST /api/web/verify-token es para credenciales de cuenta/web y usa X-Internal-Key interno. No es el verificador de tokenUserGameId opaco. No solicitar una clave global para solucionar su rechazo. El ejemplo anterior de MD5/minuto pertenece al contrato legacy de juegos sin opaque-v1 y no debe implementarse como mecanismo moderno de identidad.

## Shop, pagos y vídeo

Leer `shop-and-ads.md` y `GAME_INTEGRATION.md`: contienen llamadas concretas de apertura de tienda F2P, Pipe para anuncios, callbacks, claves privadas, IDs, cantidades, idempotencia, sandbox y pruebas. El callback local PurchaseOk no acredita entrega ni pago. Ack sólo después de la confirmación duradera del backend.
