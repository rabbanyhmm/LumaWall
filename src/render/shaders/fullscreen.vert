#version 450

layout(location = 0) out vec2 outUV;

layout(push_constant) uniform PushConstants {
    vec2 scale;
    vec2 offset;
} push;

void main() {
    // Generate a fullscreen triangle
    vec2 baseUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    outUV = baseUV * push.scale + push.offset;
    gl_Position = vec4(baseUV * 2.0f - 1.0f, 0.0f, 1.0f);
}
