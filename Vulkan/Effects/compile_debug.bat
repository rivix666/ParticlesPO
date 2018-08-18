glslangValidator.exe -g -o baseFrag.spv -V ./source/baseEffect.frag
glslangValidator.exe -g -o baseVert.spv -V ./source/baseEffect.vert
glslangValidator.exe -g -o particleFrag.spv -V ./source/particleEffect.frag
glslangValidator.exe -g -o particleVert.spv -V ./source/particleEffect.vert
glslangValidator.exe -g -o particleGeom.spv -V ./source/particleEffect.geom

REM glslangValidator.exe -o particleComp.spv -V ./source/particleEffect.Comp