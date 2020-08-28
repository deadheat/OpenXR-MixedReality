$input v_positionWorld, v_normalWorld
#include "common.sh"

uniform mat4 u_viewProjection;
uniform vec4 u_eyePosition;
uniform mat3 u_highlightPositionLightDirectionLightColor;
uniform vec4 u_numSpecularMipLevelsAnimationTime;


#define u_highlightPosition u_highlightPositionLightDirectionLightColor[0]
#define u_lightDirection u_highlightPositionLightDirectionLightColor[1]
#define u_lightColor u_highlightPositionLightDirectionLightColor[2]
#define u_numSpecularMipLevels u_numSpecularMipLevelsAnimationTime.x
#define u_animationTime u_numSpecularMipLevelsAnimationTime.y


float PI = 3.141592653589793;

float HighlightFromLocation(vec3 _sourcePosition, float _animationTime, vec3 _position)
{
    float rippleSpeed = 2.0; // m/s
    float smoothStart = _animationTime * rippleSpeed - 1.25;
    float smoothEnd = _animationTime * rippleSpeed;
    return 1.0 - smoothstep(smoothStart, smoothEnd, distance(_sourcePosition, _position));
}

void main()
{
    vec3 highlightColor = vec3(1.0, 1.0, 1.0);
    float timemax = 15.0;
    float animationTime1 = min(max(0.01, u_animationTime + 0.25), 10.0);
    float animationTime2 = min(max(0.01, u_animationTime - 0.5), 10.0);

    float spot1 = HighlightFromLocation(u_highlightPosition, animationTime1, v_positionWorld);
    float spot2 = HighlightFromLocation(u_highlightPosition, animationTime2, v_positionWorld);

    gl_FragColor = vec4(highlightColor * (v_normalWorld.y * 0.5 + 0.5) * (spot1 - spot2), 1.0);
}
