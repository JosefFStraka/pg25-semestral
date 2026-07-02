#pragma once
#include <array>
#include <unordered_map>
#include <unordered_set>

#include "ModelInstance.hpp"
#include "Camera.hpp"
#include "Skybox.hpp"

// SoA
constexpr int MAX_LIGHTS = 16;
struct s_lights {
  std::array<glm::vec4, MAX_LIGHTS> position;
  std::array<glm::vec4, MAX_LIGHTS> direction;
  std::array<glm::vec4, MAX_LIGHTS> color;
  std::array<float, MAX_LIGHTS> attenuation;
  std::array<float, MAX_LIGHTS> spotCutoff;
};

#include "engine/resources/AssetManager.hpp"
#include <format>

struct Scene {
private:
  std::unordered_map<std::string, ModelInstance> models;
  std::unordered_map<std::string, ModelInstance *> enabled_models;
  std::unordered_map<std::string, std::vector<AABB>> static_aabbs;

  std::unordered_map<std::string, ModelInstance *> enabled_dynamic_models;
  std::unordered_map<std::string, std::vector<AABB> *> enabled_static_aabbs;

  void load_static_aabbs(ModelInstance *model_inst_ptr,
                         std::vector<AABB> &aabbs, AssetManager &assets) {
    auto model_res = assets.getResourceMaybe(model_inst_ptr->model);
    if (model_res && !model_res->meshes.empty()) {
      bool all_loaded = true;
      for (auto &mesh_pkg : model_res->meshes) {
        if (!assets.getResourceMaybe(mesh_pkg.mesh)) {
          all_loaded = false;
          break;
        }
      }
      if (all_loaded) {
        for (auto &mesh_pkg : model_res->meshes) {
          auto mesh = assets.getResourceMaybe(mesh_pkg.mesh);
          glm::mat4 local_transform = model_inst_ptr->createMM(
              mesh_pkg.origin, mesh_pkg.eulerAngles, mesh_pkg.scale);
          glm::mat4 global_transform =
              local_transform * model_inst_ptr->local_model_matrix;
          aabbs.push_back(mesh->aabb_.transform(global_transform));
        }
      }
    }
  }

  bool check_dynamic_collision(ModelInstance *model_inst_ptr,
                               const AABB &bounds, AssetManager &assets) {
    auto model_res = assets.getResourceMaybe(model_inst_ptr->model);
    if (!model_res)
      return false;

    model_inst_ptr->prepare();
    for (auto &mesh_pkg : model_res->meshes) {
      auto mesh = assets.getResourceMaybe(mesh_pkg.mesh);
      if (!mesh)
        continue;

      glm::mat4 local_transform = model_inst_ptr->createMM(
          mesh_pkg.origin, mesh_pkg.eulerAngles, mesh_pkg.scale);
      glm::mat4 global_transform =
          local_transform * model_inst_ptr->local_model_matrix;

      AABB global_aabb = mesh->aabb_.transform(global_transform);
      if (bounds.intersects(global_aabb)) {
        return true;
      }
    }
    return false;
  }

public:
  std::shared_ptr<Camera> camera;
  std::shared_ptr<Skybox> skybox;
  s_lights lights;
  int active_lights{0};

  std::unordered_map<std::string, ModelInstance> &get_all_models() {
    return models;
  }
  std::unordered_map<std::string, ModelInstance *> &get_models() {
    return enabled_models;
  }

  void add_model(const std::string &name, ModelInstance model,
                 bool enabled = true) {
    models.emplace(name, std::move(model));
    set_model_enabled(name, enabled);
  }

  bool is_model_static(const std::string &name) const {
    return static_aabbs.contains(name);
  }

  bool is_model_enabled(const std::string &name) const {
    return enabled_models.contains(name);
  }

  void set_model_enabled(const std::string &name, bool enabled) {
    if (enabled) {
      enabled_models[name] = &models.at(name);
      if (is_model_static(name)) {
        enabled_static_aabbs[name] = &static_aabbs.at(name);
      } else {
        enabled_dynamic_models[name] = &models.at(name);
      }
    } else {
      enabled_models.erase(name);
      enabled_static_aabbs.erase(name);
      enabled_dynamic_models.erase(name);
    }
  }

  void add_static_model(const std::string &name, ModelInstance model,
                        AssetManager &assets, bool enabled = true) {
    model.prepare();
    std::vector<AABB> aabbs;
    auto model_res = assets.getResourceMaybe(model.model);
    if (model_res) {
      for (auto &mesh_pkg : model_res->meshes) {
        auto mesh = assets.getResourceMaybe(mesh_pkg.mesh);
        if (mesh) {
          glm::mat4 local_transform = model.createMM(
              mesh_pkg.origin, mesh_pkg.eulerAngles, mesh_pkg.scale);
          glm::mat4 global_transform =
              local_transform * model.local_model_matrix;
          aabbs.push_back(mesh->aabb_.transform(global_transform));
        }
      }
    }
    static_aabbs.emplace(name, std::move(aabbs));
    models.emplace(name, std::move(model));
    set_model_enabled(name, enabled);
  }

  bool check_collision(const AABB &bounds, AssetManager &assets) {
    return !get_collided_model_name(bounds, assets).empty();
  }

  std::string get_collided_model_name(const AABB &bounds, AssetManager &assets) {
    // Check static models
    for (auto &[name, aabbs_ptr] : enabled_static_aabbs) {
      if (aabbs_ptr->empty()) {
        load_static_aabbs(enabled_models.at(name), *aabbs_ptr, assets);
      }

      for (const auto &aabb : *aabbs_ptr) {
        if (bounds.intersects(aabb)) {
          return name;
        }
      }
    }

    // Check dynamic models
    for (auto &[name, model_inst_ptr] : enabled_dynamic_models) {
      if (!model_inst_ptr->collision_enabled)
        continue;
      if (check_dynamic_collision(model_inst_ptr, bounds, assets)) {
        return name;
      }
    }
    return "";
  }

  std::vector<AABB> get_model_aabbs(const std::string& name, AssetManager& assets) {
    if (static_aabbs.contains(name)) {
        if (static_aabbs[name].empty()) {
            load_static_aabbs(&models.at(name), static_aabbs[name], assets);
        }
        return static_aabbs[name];
    }
    return {};
  }

  void set_light(int i, glm::vec4 position, glm::vec4 direction,
                 glm::vec4 color, float attenuation, float spotCutoff) {
    lights.position[i] = position;
    lights.direction[i] = direction;
    lights.color[i] = color;
    lights.attenuation[i] = attenuation;
    lights.spotCutoff[i] = spotCutoff;
  }

  void remove_model(const std::string &name) {
    set_model_enabled(name, false);
    models.erase(name);
    static_aabbs.erase(name);
  }

  void update_shader_lights(ShaderProgram *shader) {
    if (shader) {
      shader->setUniform("active_lights", this->active_lights);
      shader->setUniform("lights.position", MAX_LIGHTS, &lights.position[0]);
      shader->setUniform("lights.direction", MAX_LIGHTS, &lights.direction[0]);
      shader->setUniform("lights.color", MAX_LIGHTS, &lights.color[0]);
      shader->setUniform("lights.attenuation", MAX_LIGHTS,
                         &lights.attenuation[0]);
      shader->setUniform("lights.spotCutoff", MAX_LIGHTS,
                         &lights.spotCutoff[0]);
    }
  }
};
