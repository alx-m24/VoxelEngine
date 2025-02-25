#version 430 core

out vec4 fragColor;

uniform float fov;
uniform mat3 uViewMatrix;
uniform vec2 uResolution;
uniform vec3 uOrigin;

#define MIN_POSITIVE_FLOAT 0.0001

const int ChunkSize = 16 * 4; // Number of voxels per chunk
const float VoxelSize = 1.0 / 4.0;
const int VoxelNum = ChunkSize * ChunkSize * ChunkSize;

uniform float maxDist;

const int renderRadius = 2;
const int numberOfChunksInAStraightLine = (2 * renderRadius + 1);
//const int chunkNum = numberOfChunksInAStraightLine * numberOfChunksInAStraightLine;
const int chunkNum = 1;

struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    vec3 color;

    bool shadows;
};

uniform DirLight dirlight;
uniform int viewingOptions;
uniform float reflection;
uniform int svoLimit;
uniform bool useSVO;

layout(std140, binding = 0) buffer Chunks {
    vec4 chunks[chunkNum]; // vec4(posX, posY, posZ, first svo index);
};

layout(std140, binding = 1) buffer Voxels {
    vec4 voxels[VoxelNum * chunkNum]; // vec4(r, g, b, alpha + reflection) 
};

layout(std140, binding = 2) buffer SVOs {
    vec4 svo[];
};

uint toIdx(vec3 position) {
	return int(position.x) + (int(position.y) * ChunkSize) + (int(position.z) * ChunkSize * ChunkSize);
}
vec3 toGridPos(vec3 position) {
	return position / VoxelSize;
}
bool isValid(vec3 pos) {
	return (pos.x >= 0.0 && pos.y >= 0.0 && pos.z >= 0.0) && (pos.x < ChunkSize && pos.y < ChunkSize && pos.z < ChunkSize);
}

vec4 rayMarch(vec3 origin, vec3 direction, int chunkIdx, inout float minDist, out vec3 pos, out vec3 normal);
vec4 shadowRayMarch(vec3 origin, vec3 direction, int chunkIdx, inout float minDist, out vec3 pos, out vec3 normal);
vec3 getLight(vec3 position, vec3 rayDir, vec3 normal, vec3 color, DirLight light);
vec3 getPos(vec3 chunkPos, vec3 currentIdx);

void main() {
	vec2 aspectRatio = vec2(uResolution.x / uResolution.y, 1.0);
	vec2 uv = gl_FragCoord.xy / uResolution;
	vec2 Normalized_uv = 2.0 * uv - 1.0;

	vec3 rayDirection = normalize(vec3(Normalized_uv * aspectRatio * fov, -1.0) * uViewMatrix);
    vec3 rayOrigin = uOrigin;

    vec4 color;
    vec3 pos, normal;
    float minDist = maxDist;
    float lastminDist = minDist;
    float transparentDist = minDist;
    
    for (int i = 0; i < chunkNum; ++i) {
        vec4 newColor = rayMarch(rayOrigin, rayDirection, i, minDist, pos, normal);
        if (newColor.a == 1.0) {
            if (minDist < transparentDist) color = newColor;
            else color = vec4(mix(newColor.rgb, color.rgb, color.a), 1.0);
        }
        else if (newColor.a > 0.0) {
            color = vec4(mix(getLight(pos, rayDirection, normal, color.rgb, dirlight), newColor.rgb, newColor.a), min(newColor.a + color.a, 1.0));
            transparentDist = minDist;
            minDist = lastminDist;          
        }
        lastminDist =  minDist;
    }

    float normalizedDist = distance(pos, rayOrigin) / maxDist;

    if (viewingOptions == 0) {
        if (color.a == 0.0) discard;
        if (color.a == 1.0) fragColor = vec4(getLight(pos, rayDirection, normal, color.rgb, dirlight), 1.0);
        else fragColor = vec4(color.rgb, 1.0);
    }
    else if (viewingOptions == 1) {
        if (color.a == 0.0) discard;
	    fragColor = vec4(color.rgb, 1.0);
    }
    else if (viewingOptions == 2) {
        if (color.a == 0.0) discard;
	    fragColor = vec4(normal.rgb, 1.0);
    }
    else if (viewingOptions == 3) {
        if (color.a == 0.0) normalizedDist = 1.0;
        fragColor = vec4(vec3(normalizedDist), 1.0);
    }
}

bool isValidForSVO(vec3 idx) {
    return (idx.x >= 0.0 && idx.y >= 0.0 && idx.z >= 0.0) && (idx.x < 2 && idx.y < 2 && idx.z < 2);
}

vec4 traverseSVO(vec3 origin, vec3 direction, int chunkIdx, float minDist, inout float Min, inout float Max) {
    int svoIdx = int(chunks[chunkIdx].a);

    vec3 chunkPos = svo[svoIdx].xyz;
    float size = svo[svoIdx].a;

    if (size < 0.0) return vec4(0.0);

    size = abs(size);

    vec3 minChunkPos = chunkPos - VoxelSize;
    vec3 maxChunkPos = minChunkPos + vec3(size);

    vec3 dirfrac = 1.0 / direction;

    float t1 = (minChunkPos.x - origin.x) * dirfrac.x;
	float t2 = (maxChunkPos.x - origin.x) * dirfrac.x;
	float t3 = (minChunkPos.y - origin.y) * dirfrac.y;
	float t4 = (maxChunkPos.y - origin.y) * dirfrac.y;
	float t5 = (minChunkPos.z - origin.z) * dirfrac.z;
	float t6 = (maxChunkPos.z - origin.z) * dirfrac.z;

	float tmin = max(max(min(t1, t2), min(t3, t4)), min(t5, t6)) + VoxelSize;
	float tmax = min(min(max(t1, t2), max(t3, t4)), max(t5, t6)) - VoxelSize;

	// if tmax < 0, ray (line) is intersecting AABB, but the whole AABB is behind us
	if (tmax < 0.0)
	{
		return vec4(0.0);
	}

	// if tmin > tmax, ray doesn't intersect AABB
	if (tmin > tmax)
	{
		return vec4(0.0);
	}

    if (tmin > minDist)
    {
        return vec4(0.0);
    }

    Min = max(0.0, tmin);
    Max = min(minDist, tmax);

    vec3 tMax;
    ivec3 step;
    vec3 delta;
    vec3 position = origin + direction * Min;

    bool traversing = useSVO;
    bool firstPass = true;
    
    while (traversing) {
        size = abs(svo[svoIdx].a) / 2.0;
        chunkPos = svo[svoIdx].xyz - VoxelSize + size;

        if (size / VoxelSize < svoLimit) break;

        vec3 currentIdx = ceil((position - chunkPos) / size);
	    vec3 previousIdx = currentIdx;

        if (direction.x > 0.0) {
            step.x = 1;
            delta.x = size * dirfrac.x;
            previousIdx.x = currentIdx.x + 1;
            tMax.x = Min + (chunkPos.x + currentIdx.x * size
                            - position.x) * dirfrac.x;
        } else if (direction.x < 0.0) {
            step.x = -1;
            delta.x = size * -dirfrac.x; 
            previousIdx.x = currentIdx.x - 1;
            tMax.x = Min + (chunkPos.x + previousIdx.x * size
                            - position.x) * dirfrac.x;
        } else {
            step.x = 0;
            delta.x = Max;
            tMax.x = Max;
        }

        if (direction.y > 0.0) {
            step.y = 1;
            delta.y = size * dirfrac.y;
            previousIdx.y = currentIdx.y + 1;
            tMax.y = Min + (chunkPos.y + currentIdx.y * size
                            - position.y) * dirfrac.y;
        } else if (direction.y < 0.0) {
            step.y = -1;
            delta.y = size * -dirfrac.y;
            previousIdx.y = currentIdx.y - 1;
            tMax.y = Min + (chunkPos.y + previousIdx.y * size
                            - position.y) * dirfrac.y;
        } else {
            step.y = 0;
            delta.y = 0.0;
            tMax.y = 0.0;
        }

        if (direction.z > 0.0) {
            step.z = 1;
            delta.z = size * dirfrac.z;
            previousIdx.z = currentIdx.z + 1;
            tMax.z = Min + (chunkPos.z + currentIdx.z * size
                        - position.z) * dirfrac.z;
        } else if (direction.z < 0.0) {
            step.z = -1;
            delta.z = size * -dirfrac.z;
            previousIdx.z = currentIdx.z - 1;
            tMax.z = Min + (chunkPos.z + previousIdx.z * size
                        - position.z) * dirfrac.z;
        } else {
            step.z = 0;
            delta.z = 0.0;
            tMax.z = 0.0;
        }

        while (isValidForSVO(currentIdx)) {
            int index = int(currentIdx.x) * 4 + int(currentIdx.y) * 2 + int(currentIdx.z);
            //int index = int(currentIdx.x) + int(currentIdx.y) * 2 + int(currentIdx.z) * 4;

            int childNum = index + 1;

            int generalArrayIdx = svoIdx * 8 + childNum;

            if (childNum == 1) return vec4(1.0, 0.0, 0.0, 1.0);
            if (childNum == 2) return vec4(0.0, 1.0, 0.0, 1.0);
            if (childNum == 3) return vec4(0.0, 0.0, 1.0, 1.0);
            if (childNum == 4) return vec4(1.0, 0.0, 1.0, 1.0);
            if (childNum == 5) return vec4(0.0, 1.0, 1.0, 1.0);
            if (childNum == 6) return vec4(0.5, 0.5, 0.5, 1.0);
            if (childNum == 7) return vec4(0.5, 0.0, 1.0, 1.0);
            if (childNum == 8) return vec4(0.0, 0.5, 1.0, 1.0);

            if (svo[generalArrayIdx].a >= 0.0) {
                svoIdx = generalArrayIdx;
                break;
            }

            vec3 lastPos = position;

            if (tMax.x < tMax.y && tMax.x < tMax.z) {
                // X-axis traversal.
                currentIdx.x += step.x;
                tMax.x += delta.x;
                position.x += delta.x;
                
		    } else if (tMax.y < tMax.z) {
                // Y-axis traversal.
		    	currentIdx.y += step.y;
                tMax.y += delta.y;
                position.y += delta.y;
            } else {
                // Z-axis traversal.
                currentIdx.z += step.z;
                tMax.z += delta.z;
                position.z += delta.z;
            }
            
            if (!isValidForSVO(currentIdx)) {
                position = lastPos;
                traversing = false;
                return vec4(0.0, 1.0, 0.0, 1.0);
                break;
            }
        }
        firstPass = false;
        return vec4(1.0);
    }

    return vec4(1.0);
    return vec4(position, 1.0);
}

bool inShadow(vec3 position, vec3 lightDir) {
    position += lightDir * VoxelSize;

    vec3 pos, normal;
    float minDist = maxDist;
    for (int i = 0; i < chunkNum; ++i) {
        vec4 newColor = shadowRayMarch(position, lightDir, i, minDist, pos, normal);
        if (newColor.a > 0.0) return true;
    }

    return false;
}

vec4 shadowRayMarch(vec3 origin, vec3 direction, int chunkIdx, inout float minDist, out vec3 pos, out vec3 normal) {
    if (direction.x == 0.0) direction.x = MIN_POSITIVE_FLOAT;
    if (direction.y == 0.0) direction.y = MIN_POSITIVE_FLOAT;
    if (direction.z == 0.0) direction.z = MIN_POSITIVE_FLOAT;

    vec3 chunkPos = chunks[chunkIdx].xyz;
    vec3 minChunkPos = chunkPos - VoxelSize;
    vec3 maxChunkPos = chunkPos + vec3(ChunkSize * VoxelSize) - VoxelSize;

    vec3 dirfrac = 1.0 / direction;

    float t1 = (minChunkPos.x - origin.x) * dirfrac.x;
	float t2 = (maxChunkPos.x - origin.x) * dirfrac.x;
	float t3 = (minChunkPos.y - origin.y) * dirfrac.y;
	float t4 = (maxChunkPos.y - origin.y) * dirfrac.y;
	float t5 = (minChunkPos.z - origin.z) * dirfrac.z;
	float t6 = (maxChunkPos.z - origin.z) * dirfrac.z;

	float tmin = max(max(min(t1, t2), min(t3, t4)), min(t5, t6)) - VoxelSize;
	float tmax = min(min(max(t1, t2), max(t3, t4)), max(t5, t6)) + VoxelSize;

	// if tmax < 0, ray (line) is intersecting AABB, but the whole AABB is behind us
	if (tmax < 0.0)
	{
		return vec4(0.0);
	}

	// if tmin > tmax, ray doesn't intersect AABB
	if (tmin > tmax)
	{
		return vec4(0.0);
	}

    if (tmin > minDist)
    {
        return vec4(0.0);
    }

    float Min = max(0.0, tmin);
    float Max = min(minDist, tmax);

    vec3 position = origin + direction * Min;

	vec3 currentIdx = ceil((position - chunkPos) / VoxelSize);
	vec3 previousIdx = currentIdx;

	vec3 tMax;
	vec3 delta;
    vec3 step;

	if (direction.x > 0.0) {
        step.x = 1;
        delta.x = VoxelSize * dirfrac.x;
        previousIdx.x = currentIdx.x + 1;
        tMax.x = Min + (chunkPos.x + currentIdx.x * VoxelSize
                        - position.x) * dirfrac.x;
    } else if (direction.x < 0.0) {
        step.x = -1;
        delta.x = VoxelSize * -dirfrac.x; 
        previousIdx.x = currentIdx.x - 1;
        tMax.x = Min + (chunkPos.x + previousIdx.x * VoxelSize
                        - position.x) * dirfrac.x;
    } else {
        step.x = 0;
        delta.x = Max;
        tMax.x = Max;
    }

    if (direction.y > 0.0) {
        step.y = 1;
        delta.y = VoxelSize * dirfrac.y;
        previousIdx.y = currentIdx.y + 1;
        tMax.y = Min + (chunkPos.y + currentIdx.y * VoxelSize
                        - position.y) * dirfrac.y;
    } else if (direction.y < 0.0) {
        step.y = -1;
        delta.y = VoxelSize * -dirfrac.y;
        previousIdx.y = currentIdx.y - 1;
        tMax.y = Min + (chunkPos.y + previousIdx.y * VoxelSize
                        - position.y) * dirfrac.y;
    } else {
        step.y = 0;
        delta.y = 0.0;
        tMax.y = 0.0;
    }

    if (direction.z > 0.0) {
        step.z = 1;
        delta.z = VoxelSize * dirfrac.z;
        previousIdx.z = currentIdx.z + 1;
        tMax.z = Min + (chunkPos.z + currentIdx.z * VoxelSize
                    - position.z) * dirfrac.z;
    } else if (direction.z < 0.0) {
        step.z = -1;
        delta.z = VoxelSize * -dirfrac.z;
        previousIdx.z = currentIdx.z - 1;
        tMax.z = Min + (chunkPos.z + previousIdx.z * VoxelSize
                    - position.z) * dirfrac.z;
    } else {
        step.z = 0;
        delta.z = 0.0;
        tMax.z = 0.0;
    }

    float dist = Min;

    // Seems like dist is never greater than max;
	while (dist < Max) {
        if (isValid(currentIdx)) {
            uint idx = toIdx(currentIdx) + (chunkIdx * VoxelNum);
            if (voxels[idx].a > 0.0) return vec4(1.0);
        }

        previousIdx = currentIdx;
		
		if (tMax.x < tMax.y && tMax.x < tMax.z) {
            // X-axis traversal.
            currentIdx.x += step.x;
            tMax.x += delta.x;
		} else if (tMax.y < tMax.z) {
            // Y-axis traversal.
			currentIdx.y += step.y;
            tMax.y += delta.y;
        } else {
            // Z-axis traversal.
            currentIdx.z += step.z;
            tMax.z += delta.z;
        }

        position = getPos(chunkPos, currentIdx);
        dist = distance(position, origin);
	}

    return vec4(0.0);
}

vec3 getLight(vec3 position, vec3 rayDir, vec3 normal, vec3 color, DirLight light) {    
    light.ambient *= light.color;
    light.diffuse *= light.color;
    light.specular *= light.color;

    vec3 lightDir = normalize(-light.direction);

    vec3 ambient = light.ambient * color;

    if (light.shadows) {
        if (inShadow(position, lightDir)) return ambient;
    }

    vec3 halfwayDir = normalize(lightDir - rayDir);

    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);

    // TODO: change reflection value
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 100.0);

    // combine results
    vec3 diffuse = light.diffuse * diff * color;
    vec3 specular = light.specular * spec * color;

    return ambient + diffuse + specular;
}

vec4 rayMarch(vec3 origin, vec3 direction, int chunkIdx, inout float minDist, out vec3 pos, out vec3 normal) {
    vec3 chunkPos = chunks[chunkIdx].xyz;
    vec3 dirfrac = 1.0 / direction;

    float Min;
    float Max;

    vec4 temp = traverseSVO(origin, direction, chunkIdx, minDist, Min, Max);
    if (temp.a == 0.0) return vec4(0.0);

    return temp;
    
    vec3 position = temp.xyz;

	vec3 currentIdx = ceil((position - chunkPos) / VoxelSize);
	vec3 previousIdx = currentIdx;

	vec3 tMax;
	vec3 delta;
    vec3 step;

	if (direction.x > 0.0) {
        step.x = 1;
        delta.x = VoxelSize * dirfrac.x;
        previousIdx.x = currentIdx.x + 1;
        tMax.x = Min + (chunkPos.x + currentIdx.x * VoxelSize
                        - position.x) * dirfrac.x;
    } else if (direction.x < 0.0) {
        step.x = -1;
        delta.x = VoxelSize * -dirfrac.x; 
        previousIdx.x = currentIdx.x - 1;
        tMax.x = Min + (chunkPos.x + previousIdx.x * VoxelSize
                        - position.x) * dirfrac.x;
    } else {
        step.x = 0;
        delta.x = Max;
        tMax.x = Max;
    }

    if (direction.y > 0.0) {
        step.y = 1;
        delta.y = VoxelSize * dirfrac.y;
        previousIdx.y = currentIdx.y + 1;
        tMax.y = Min + (chunkPos.y + currentIdx.y * VoxelSize
                        - position.y) * dirfrac.y;
    } else if (direction.y < 0.0) {
        step.y = -1;
        delta.y = VoxelSize * -dirfrac.y;
        previousIdx.y = currentIdx.y - 1;
        tMax.y = Min + (chunkPos.y + previousIdx.y * VoxelSize
                        - position.y) * dirfrac.y;
    } else {
        step.y = 0;
        delta.y = 0.0;
        tMax.y = 0.0;
    }

    if (direction.z > 0.0) {
        step.z = 1;
        delta.z = VoxelSize * dirfrac.z;
        previousIdx.z = currentIdx.z + 1;
        tMax.z = Min + (chunkPos.z + currentIdx.z * VoxelSize
                    - position.z) * dirfrac.z;
    } else if (direction.z < 0.0) {
        step.z = -1;
        delta.z = VoxelSize * -dirfrac.z;
        previousIdx.z = currentIdx.z - 1;
        tMax.z = Min + (chunkPos.z + previousIdx.z * VoxelSize
                    - position.z) * dirfrac.z;
    } else {
        step.z = 0;
        delta.z = 0.0;
        tMax.z = 0.0;
    }

    float dist = Min;

    vec4 color = vec4(0.0);

	while (dist < Max) {
        if (isValid(currentIdx)) {
            uint idx = toIdx(currentIdx) + (chunkIdx * VoxelNum);
            vec4 voxel = voxels[idx];
            if (voxel.a == 1.0) { 
                minDist = dist;
                normal = previousIdx - currentIdx;                    
                pos = position;
                return vec4(mix(voxel.rgb, color.rgb, color.a), voxel.a);
            } 
            else if (voxel.a > 0.0) {
                if (viewingOptions != 0) return voxel;

                color = vec4(mix(getLight(position, direction, previousIdx - currentIdx, voxel.rgb, dirlight), color.rgb, color.a), voxel.a + color.a);
                minDist = dist;

                if (color.a >= 1.0) {
                    normal = previousIdx - currentIdx;                    
                    pos = position;
                    return vec4(color.rgb, 1.0);
                }
            }
        }

        previousIdx = currentIdx;
		
		if (tMax.x < tMax.y && tMax.x < tMax.z) {
            // X-axis traversal.
            currentIdx.x += step.x;
            tMax.x += delta.x;
		} else if (tMax.y < tMax.z) {
            // Y-axis traversal.
			currentIdx.y += step.y;
            tMax.y += delta.y;
        } else {
            // Z-axis traversal.
            currentIdx.z += step.z;
            tMax.z += delta.z;
        }

        position = getPos(chunkPos, currentIdx);
        dist = distance(position, origin);
	}

    return vec4(mix(vec3(0.0), color.rgb, color.a), color.a);
}

vec3 getPos(vec3 chunkPos, vec3 currentIdx) {
    return (VoxelSize * currentIdx) + chunkPos;
}