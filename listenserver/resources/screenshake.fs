#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform float shakeStrength;
uniform float time;

out vec4 finalColor;

void main() {
    vec2 uv = fragTexCoord;

    if (shakeStrength > 0.0) {
        uv.x += sin(time * 47.0) * shakeStrength;
        uv.y += cos(time * 31.0) * shakeStrength;
    }

    finalColor = texture(texture0, uv) * fragColor;
}