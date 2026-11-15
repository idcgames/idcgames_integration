# Biblioteca nativa IDC Games

`runtimes/win-x64/native/IDCLib2.dll` se compila en Release desde las fuentes de este repositorio y lleva firma Authenticode con sello de tiempo. Arquitectura PE: x64. Tamaño: 191440 bytes. SHA-256:

```text
b217efedae3ab896740dc55bf0bec6e33cc4b76f98b16b2b6bd4c1fbb5df5cf6
```

El [manifiesto](../runtimes/win-x64/native/manifest.json) registra la procedencia. La comprobación automática carga el binario y resuelve sus exports sin iniciar comunicación con el Launcher.

Para compilar las fuentes: abrir `IDCLib2/IDCLib2.sln` con Visual Studio y herramientas C++ de Windows; el proyecto original usa toolset v142. Seleccionar Release/x64, instalar el Windows SDK requerido por el proyecto o retargetearlo conscientemente. Revisar y probar cualquier binario recompilado antes de sustituir el distribuido y actualizar el manifiesto.

El código nativo heredado contiene buffers fijos y mecanismos de compatibilidad históricos. El wrapper aplica límites a sus entradas, pero no convierte el IPC local en un protocolo autenticado ni constituye una auditoría completa del código nativo. No cargar claves de servicios internos en esta biblioteca. Las condiciones de redistribución del binario deben confirmarse con IDC Games; se conserva la licencia original del repositorio.
