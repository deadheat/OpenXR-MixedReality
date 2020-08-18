////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation.  All Rights Reserved
// Licensed under the MIT License. See License.txt in the project root for license information.
$input v_positionProj, v_positionWorld, v_normalWorld
#include "HighlightShared.sc"

const float PI = 3.141592653589793;

float HighlightFromLocation(vec3 sourcePosition, float animationTime, vec3 position)
{
    float rippleSpeed = 2; // m/s
    float smoothStart = animationTime * rippleSpeed - 1.25;
    float smoothEnd = animationTime * rippleSpeed;
    return 1 - smoothstep(smoothStart, smoothEnd, distance(sourcePosition, position));
}

float4 main()
{
    const vec3 highlightColor = vec3_splat(1.0f, 1.0f, 1.0f);
    const float timemax = 15;
    const float animationTime1 = min(max(0.01, u_animationTime + 0.25), 10);
    const float animationTime2 = min(max(0.01, u_animationTime - 0.5), 10);

    const float spot1 = HighlightFromLocation(u_highlightPosition, animationTime1, v_positionWorld);
    const float spot2 = HighlightFromLocation(u_highlightPosition, animationTime2, v_positionWorld);

    gl_FragColor = vec4_splat(highlightColor * (v_normalWorld.y * 0.5f + 0.5f) * (spot1 - spot2), 1.0f);
}
