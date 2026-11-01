#version 440

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D trajectoryTex;

void main() {
    vec4 texColor = texture(trajectoryTex, texCoord);
    
    // 只输出纹理的alpha通道作为透明度
    // 绿色线条部分完全不透明，透明部分完全透明
    fragColor = vec4(texColor.rgb, texColor.a);
}
