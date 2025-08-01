# MobileGlues

**MobileGlues**, which stands for "(on) Mobile, GL uses ES", is a GL implementation running on top of host OpenGL ES 3.x (best on 3.2, minimum 3.0), with running Minecraft: Java Edition in mind.

# For Shader Developers

1. MobileGlues automatically:
   - Converts desktop GLSL → GLSL ES
   - Removes `layout(binding)` syntax
   - Handles version directives
   - Always declare precision explicitly:
     ```glsl
     precision highp float;
     precision highp int;
     ```

2. MobileGlues (since V1.2.6.0) injects these macros into your shaders:
   ```glsl
   #define MG_MOBILEGLUES                   // Indicates MobileGlues environment
   #define MG_MOBILEGLUES_VERSION 1260      // Version number (e.g. 1260 = V1.2.6)
   ```

   Use these macros for platform-specific logic:
   ```glsl
   #ifdef MG_MOBILEGLUES
       #if MG_MOBILEGLUES_VERSION >= 1270
           // Logic for MobileGlues (version >= V1.2.7)
       #else
           // Logic for MobileGlues (version < V1.2.7)
       #endif
   #else
       // ...
   #endif
   ```

3. If encountering issues:
   - Enable `Ignore shader/program error`, and check the logs (located at `/sdcard/MG/latest.log`).

# License

MobileGlues is licensed under **GNU LGPL-2.1 License**.

Please see [LICENSE](https://github.com/MobileGL-Dev/MobileGlues/blob/main/LICENSE).

# Third party components

**SPIRV-Cross** by **KhronosGroup** - [Apache License 2.0](https://github.com/KhronosGroup/SPIRV-Cross/blob/master/LICENSE): [github](https://github.com/KhronosGroup/SPIRV-Cross)

**glslang** by **KhronosGroup** - [Various Licenses](https://github.com/KhronosGroup/glslang/blob/main/LICENSE.txt): [github](https://github.com/KhronosGroup/glslang)

**GlslOptimizerV2** by **aiekick** - [Apache License 2.0](https://github.com/aiekick/GlslOptimizerV2/blob/master/LICENSE): [github](https://github.com/aiekick/GlslOptimizerV2)

**cJSON** by **DaveGamble** - [MIT License](https://github.com/DaveGamble/cJSON/blob/master/LICENSE): [github](https://github.com/DaveGamble/cJSON)

**OpenGL Mathematics (*GLM*)** by **G-Truc Creation** - [The Happy Bunny License](https://github.com/g-truc/glm/blob/master/copying.txt): [github](https://github.com/g-truc/glm)

**FidelityFX-FSR** by **AMD** - [MIT License](https://github.com/GPUOpen-Effects/FidelityFX-FSR/blob/master/license.txt): [github](https://github.com/GPUOpen-Effects/FidelityFX-FSR) 
