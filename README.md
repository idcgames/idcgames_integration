# Integración oficial de IDC Games

Guía y biblioteca de referencia para integrar **login de IDC Games, Launcher, tienda y anuncios de vídeo** en un juego, sin depender de GamePanel.

## Inicio rápido

Para compilar o usar `dotnet run`: Windows x64, .NET SDK 8 o posterior y runtime .NET 8 x64 para ejecutar el ejemplo. Si ya tenéis Visual Studio con esos componentes, no hace falta instalarlos de nuevo.

Para ejecutar el [ejemplo ya compilado](https://github.com/idcgames/idcgames_integration/releases/tag/v2.0.0), descargar y descomprimir el ZIP completo y ejecutar `LauncherDemo.exe --demo`: basta el runtime .NET 8 x64, sin SDK. Esta dependencia pertenece al ejemplo C#, no a la DLL nativa de IDC Games.

```powershell
dotnet run --project examples/LauncherDemo -- --demo
dotnet run --project examples/LauncherDemo -- --check-native
```

El primer comando muestra eventos ficticios offline. El segundo carga la DLL y comprueba sus exports, sin conectar al Launcher. Para conectar un juego real, solicitar a IDC Games su ID, clave cliente del Launcher y configuración de servicios, y seguir [el ejemplo C#](docs/quickstart-csharp.md).

## Guías de integración

- [Login e identidad IDC Games](docs/authentication-idc.md): flujo, credenciales, peticiones y validación en backend.
- [Ejemplo C# paso a paso](docs/quickstart-csharp.md): configuración, polling y comandos de consola.
- [Tienda, pagos y vídeo Ads](docs/shop-and-ads.md): checkout, confirmación, sandbox y estados de anuncios.
- [Protocolo y ABI](docs/launcher-protocol.md): exports, callbacks y formatos de datos.
- [DLL y fuentes nativas](native/README.md): procedencia, hash y compilación.

## Biblioteca y ejemplo

| Carpeta | Contenido |
|---|---|
| `src/IDCGames.Integration` | Wrapper C# .NET 8 para IDCLib2 |
| `examples/LauncherDemo` | Login, tienda y anuncios en una consola; sandbox por defecto |
| `runtimes/win-x64/native` | IDCLib2.dll x64 y manifiesto SHA-256 |
| `native/IDCLib2` | Fuentes C++ de referencia del Launcher |
| `tests/IDCGames.Integration.Tests` | Pruebas offline de protocolo y seguridad |
| `tools` | Verificación y generación manual de paquetes |

## Verificar y distribuir

```powershell
powershell -ExecutionPolicy Bypass -File tools/verify.ps1
powershell -ExecutionPolicy Bypass -File tools/build.ps1
```

Se generan un paquete NuGet local y un ZIP de la consola en `artifacts/`. La consola publicada requiere el runtime base .NET 8 x64. No se publica automáticamente en NuGet y no hay workflows de GitHub Actions.

La inicialización nativa no equivale a login. Los callbacks locales no prueban identidad ante el servidor, pago ni derecho a recompensa; seguir las guías de validación e idempotencia. La prueba real requiere el Launcher, las cuentas y la configuración del juego. No hay credenciales reales incluidas.

Se conserva la licencia [GPL-3.0](LICENSE) del material de origen. Confirmar con IDC Games las condiciones de redistribución del binario y su uso en el producto. El código nativo se importa como referencia; la DLL distribuida se verifica por hash y exports, sin afirmar una compilación reproducible ni una auditoría completa del código heredado.
