/**
 * @file glbimporter.cpp
 * @brief Implementation of GlbImporter using cgltf. See glbimporter.h.
 */

#include "glbimporter.h"
#include "cgltf.h"
#include "humanoidautorigger.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace pose {

namespace {

std::string getBaseDir(const std::string& path) {
    try {
        std::filesystem::path p(path);
        return p.parent_path().string();
    } catch (...) {
        return "";
    }
}

TextureSource extractTextureSource(const cgltf_texture* tex, const std::string& baseDir) {
    TextureSource src;
    if (!tex || !tex->image) {
        return src;
    }
    const cgltf_image* img = tex->image;
    if (img->buffer_view && img->buffer_view->buffer && img->buffer_view->buffer->data) {
        const uint8_t* ptr = static_cast<const uint8_t*>(img->buffer_view->buffer->data) + img->buffer_view->offset;
        const std::size_t size = img->buffer_view->size;
        if (size > 0) {
            src.encoded.assign(ptr, ptr + size);
        }
    } else if (img->uri && std::strlen(img->uri) > 0) {
        std::filesystem::path p(img->uri);
        if (p.is_absolute()) {
            src.path = p.string();
        } else if (!baseDir.empty()) {
            src.path = (std::filesystem::path(baseDir) / p).string();
        } else {
            src.path = img->uri;
        }
    }
    return src;
}

void computeSmoothNormals(MeshData& mesh) {
    for (Vertex& vert : mesh.vertices) {
        vert.normal = glm::vec3(0.0f);
    }
    for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        const uint32_t i0 = mesh.indices[i];
        const uint32_t i1 = mesh.indices[i + 1];
        const uint32_t i2 = mesh.indices[i + 2];
        if (i0 >= mesh.vertices.size() || i1 >= mesh.vertices.size() || i2 >= mesh.vertices.size()) {
            continue;
        }
        const glm::vec3& p0 = mesh.vertices[i0].pos;
        const glm::vec3& p1 = mesh.vertices[i1].pos;
        const glm::vec3& p2 = mesh.vertices[i2].pos;
        const glm::vec3 fn = glm::cross(p1 - p0, p2 - p0);
        mesh.vertices[i0].normal += fn;
        mesh.vertices[i1].normal += fn;
        mesh.vertices[i2].normal += fn;
    }
    for (Vertex& vert : mesh.vertices) {
        const float len = glm::length(vert.normal);
        vert.normal = (len > 1e-6f) ? vert.normal / len : glm::vec3(0.0f, 1.0f, 0.0f);
    }
}

} // namespace

ModelData GlbImporter::load(const std::string& path) const {
    cgltf_options options{};
    cgltf_data* data = nullptr;

    cgltf_result res = cgltf_parse_file(&options, path.c_str(), &data);
    if (res != cgltf_result_success || !data) {
        throw std::runtime_error("GlbImporter: failed to parse glTF file: " + path);
    }

    res = cgltf_load_buffers(&options, data, path.c_str());
    if (res != cgltf_result_success) {
        cgltf_free(data);
        throw std::runtime_error("GlbImporter: failed to load glTF buffers: " + path);
    }

    const std::string baseDir = getBaseDir(path);
    ModelData model;

    // 1. Extract Meshes and Primitives
    for (cgltf_size m = 0; m < data->meshes_count; ++m) {
        const cgltf_mesh& gltfMesh = data->meshes[m];

        for (cgltf_size p = 0; p < gltfMesh.primitives_count; ++p) {
            const cgltf_primitive& prim = gltfMesh.primitives[p];
            if (prim.type != cgltf_primitive_type_triangles) {
                continue;
            }

            MeshData mesh;

            // Extract material settings
            if (prim.material) {
                const cgltf_material* mat = prim.material;
                if (mat->has_pbr_metallic_roughness) {
                    const cgltf_pbr_metallic_roughness& pbr = mat->pbr_metallic_roughness;
                    mesh.baseColor = glm::vec3(pbr.base_color_factor[0],
                                               pbr.base_color_factor[1],
                                               pbr.base_color_factor[2]);
                    mesh.roughness = pbr.roughness_factor;
                    mesh.metalness = pbr.metallic_factor;
                    mesh.diffuse = extractTextureSource(pbr.base_color_texture.texture, baseDir);
                    mesh.roughnessMap = extractTextureSource(pbr.metallic_roughness_texture.texture, baseDir);
                }
                if (mat->alpha_mode == cgltf_alpha_mode_blend) {
                    mesh.opacity = 0.99f;
                } else if (mat->alpha_mode == cgltf_alpha_mode_mask) {
                    mesh.hasOpacityMask = true;
                }
            }

            // Find attributes
            const cgltf_accessor* posAcc = nullptr;
            const cgltf_accessor* normAcc = nullptr;
            const cgltf_accessor* uvAcc = nullptr;
            const cgltf_accessor* jointsAcc = nullptr;
            const cgltf_accessor* weightsAcc = nullptr;

            for (cgltf_size a = 0; a < prim.attributes_count; ++a) {
                const cgltf_attribute& attr = prim.attributes[a];
                if (attr.type == cgltf_attribute_type_position) {
                    posAcc = attr.data;
                } else if (attr.type == cgltf_attribute_type_normal) {
                    normAcc = attr.data;
                } else if (attr.type == cgltf_attribute_type_texcoord && attr.index == 0) {
                    uvAcc = attr.data;
                } else if (attr.type == cgltf_attribute_type_joints && attr.index == 0) {
                    jointsAcc = attr.data;
                } else if (attr.type == cgltf_attribute_type_weights && attr.index == 0) {
                    weightsAcc = attr.data;
                }
            }

            if (!posAcc) {
                continue;
            }

            const cgltf_size vertCount = posAcc->count;
            mesh.vertices.resize(vertCount);

            for (cgltf_size v = 0; v < vertCount; ++v) {
                Vertex& vert = mesh.vertices[v];
                cgltf_accessor_read_float(posAcc, v, &vert.pos.x, 3);

                if (normAcc) {
                    cgltf_accessor_read_float(normAcc, v, &vert.normal.x, 3);
                } else {
                    vert.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                }

                if (uvAcc) {
                    cgltf_accessor_read_float(uvAcc, v, &vert.uv.x, 2);
                } else {
                    vert.uv = glm::vec2(0.0f);
                }

                if (jointsAcc && weightsAcc) {
                    cgltf_uint j[4] = {0, 0, 0, 0};
                    cgltf_accessor_read_uint(jointsAcc, v, j, 4);
                    vert.joints = glm::uvec4(j[0], j[1], j[2], j[3]);

                    cgltf_accessor_read_float(weightsAcc, v, &vert.weights.x, 4);
                } else {
                    vert.joints = glm::uvec4(0);
                    vert.weights = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
                }
            }

            // Extract indices
            if (prim.indices) {
                const cgltf_size indexCount = prim.indices->count;
                mesh.indices.reserve(indexCount);
                for (cgltf_size i = 0; i < indexCount; ++i) {
                    mesh.indices.push_back(static_cast<uint32_t>(cgltf_accessor_read_index(prim.indices, i)));
                }
            } else {
                mesh.indices.reserve(vertCount);
                for (cgltf_size i = 0; i < vertCount; ++i) {
                    mesh.indices.push_back(static_cast<uint32_t>(i));
                }
            }

            if (!normAcc) {
                computeSmoothNormals(mesh);
            }

            model.meshes.push_back(std::move(mesh));
        }
    }

    // 2. Embedded Skeleton vs Auto-Rigging
    bool hasValidEmbeddedRig = false;
    if (data->skins_count > 0 && data->skins[0].joints_count >= 15) {
        const cgltf_skin& skin = data->skins[0];
        std::unordered_map<const cgltf_node*, int> nodeToBone;
        nodeToBone.reserve(skin.joints_count);

        for (cgltf_size j = 0; j < skin.joints_count; ++j) {
            nodeToBone[skin.joints[j]] = static_cast<int>(j);
        }

        std::vector<ModelBone> bones(skin.joints_count);
        for (cgltf_size j = 0; j < skin.joints_count; ++j) {
            const cgltf_node* node = skin.joints[j];
            ModelBone& mb = bones[j];
            mb.name = node->name ? node->name : ("joint_" + std::to_string(j));
            mb.parent = (node->parent && nodeToBone.count(node->parent)) ? nodeToBone[node->parent] : -1;

            float mat[16]{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
            cgltf_node_transform_world(node, mat);
            mb.bindGlobal = glm::translate(glm::mat4(1.0f), glm::vec3(mat[12], mat[13], mat[14]));
            mb.orientation = glm::vec3(0.0f);
            mb.rotationOrder = "XYZ";
            mb.rotationMin = glm::vec3(-180.0f);
            mb.rotationMax = glm::vec3(180.0f);
            mb.rotationLimited = glm::bvec3(false);
        }

        // Basic verification: does it have a root with multiple children or identifiable joints?
        if (!bones.empty()) {
            model.bones = std::move(bones);
            hasValidEmbeddedRig = true;
        }
    }

    if (!hasValidEmbeddedRig) {
        // Automatically bind standard 21-bone humanoid skeleton and distance-weighted skinning
        HumanoidAutoRigger::buildAndSkin(model);
    }

    cgltf_free(data);
    return model;
}

} // namespace pose
