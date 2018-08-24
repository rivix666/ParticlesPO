glslangValidator.exe -o baseFrag.spv -V ./source/baseEffect.frag
glslangValidator.exe -o baseVert.spv -V ./source/baseEffect.vert

glslangValidator.exe -o particleFrag.spv -V ./source/particleEffect.frag
glslangValidator.exe -o particleVert.spv -V ./source/particleEffect.vert
glslangValidator.exe -o particleGeom.spv -V ./source/particleEffect.geom
glslangValidator.exe -o particleComp.spv -V ./source/particleEffect.comp