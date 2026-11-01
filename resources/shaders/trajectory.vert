#version 440

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexTexCoord;

layout(location = 0) out vec2 texCoord;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    mat4 modelMatrix;
    mat4 modelView;
    mat4 inverseModelView;
    mat4 viewMatrix;
};

void main() {
    gl_Position = mvp * vec4(vertexPosition, 1.0);
    texCoord = vertexTexCoord;
}
