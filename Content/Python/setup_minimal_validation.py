import json
import os

import unreal


DATA_DIR = "/Game/_Main/Data"
MATERIAL_DIR = "/Game/_Main/Materials/ManualValidation"
BLUEPRINT_DIR = "/Game/_Main/Blueprint/ManualValidation"
VALIDATION_MAP = "/Game/_Main/NewMap"
VALIDATION_MAP_ASSET = "/Game/_Main/NewMap.NewMap"
DEFAULT_LEVEL_ID = "PrototypeBattle"
VALIDATION_ACTOR_PREFIX = "TTD_ManualValidation_"


MATERIAL_SPECS = {
    "ground": ("M_TTD_Ground", (0.42, 0.52, 0.42, 1.0)),
    "path": ("M_TTD_Path", (0.74, 0.74, 0.70, 1.0)),
    "wall": ("M_TTD_Wall", (0.20, 0.21, 0.23, 1.0)),
    "decor": ("M_TTD_Decor", (0.46, 0.70, 0.48, 1.0)),
    "spawn": ("M_TTD_SpawnPoint", (0.86, 0.12, 0.18, 1.0)),
    "slot": ("M_TTD_BuildSlot", (1.00, 0.76, 0.18, 1.0)),
    "high_slot": ("M_TTD_HighBuildSlot", (0.22, 0.74, 0.34, 1.0)),
    "castle": ("M_TTD_Castle", (0.12, 0.36, 0.82, 1.0)),
    "enemy": ("M_TTD_Enemy", (0.95, 0.18, 0.20, 1.0)),
    "basic_tower": ("M_TTD_BasicTower", (1.00, 0.55, 0.08, 1.0)),
    "projectile_tower": ("M_TTD_ProjectileTower", (0.36, 0.28, 0.95, 1.0)),
    "projectile": ("M_TTD_Projectile", (0.10, 0.90, 1.00, 1.0)),
}


BLUEPRINT_SPECS = {
    "map_ground": {
        "asset_name": "BP_TTD_MapGround",
        "parent": "/Script/Engine.StaticMeshActor",
        "mesh": "/Engine/BasicShapes/Cube.Cube",
        "material": "ground",
    },
    "map_path": {
        "asset_name": "BP_TTD_MapPath",
        "parent": "/Script/Engine.StaticMeshActor",
        "mesh": "/Engine/BasicShapes/Cube.Cube",
        "material": "path",
    },
    "map_wall": {
        "asset_name": "BP_TTD_MapWall",
        "parent": "/Script/Engine.StaticMeshActor",
        "mesh": "/Engine/BasicShapes/Cube.Cube",
        "material": "wall",
    },
    "map_decor": {
        "asset_name": "BP_TTD_MapDecor",
        "parent": "/Script/Engine.StaticMeshActor",
        "mesh": "/Engine/BasicShapes/Cube.Cube",
        "material": "decor",
    },
    "spawn_point": {
        "asset_name": "BP_TTD_BattleSpawnPoint",
        "parent": "/Script/ToyTowerDefense.TTDBattleSpawnPointActor",
        "material": "spawn",
    },
    "build_slot": {
        "asset_name": "BP_TTD_BuildSlot",
        "parent": "/Script/ToyTowerDefense.TTDBuildSlotActor",
        "material": "slot",
    },
    "high_build_slot": {
        "asset_name": "BP_TTD_HighBuildSlot",
        "parent": "/Script/ToyTowerDefense.TTDBuildSlotActor",
        "material": "high_slot",
    },
    "castle": {
        "asset_name": "BP_TTD_BattleCastle",
        "parent": "/Script/ToyTowerDefense.TTDBattleCastleActor",
        "material": "castle",
    },
    "enemy": {
        "asset_name": "BP_TTD_BattleEnemy",
        "parent": "/Script/ToyTowerDefense.TTDBattleEnemyActor",
        "material": "enemy",
    },
    "basic_tower": {
        "asset_name": "BP_TTD_BasicTower",
        "parent": "/Script/ToyTowerDefense.TTDBattleBuildingActor",
        "material": "basic_tower",
    },
    "projectile_tower": {
        "asset_name": "BP_TTD_ProjectileTower",
        "parent": "/Script/ToyTowerDefense.TTDBattleBuildingActor",
        "material": "projectile_tower",
    },
    "projectile": {
        "asset_name": "BP_TTD_BattleProjectile",
        "parent": "/Script/ToyTowerDefense.TTDBattleProjectileActor",
        "material": "projectile",
    },
    "battle_camera": {
        "asset_name": "BP_TTD_BattleCamera",
        "parent": "/Script/Engine.CameraActor",
    },
}


def log(message):
    unreal.log("[TTD Minimal Validation] {}".format(message))


def warn(message):
    unreal.log_warning("[TTD Minimal Validation] {}".format(message))


def snake_case(name):
    result = []
    for index, char in enumerate(name):
        if char.isupper() and index > 0:
            result.append("_")
        result.append(char.lower())
    return "".join(result)


def set_prop(obj, prop_name, value):
    candidates = [prop_name, snake_case(prop_name)]
    last_error = None
    for candidate in candidates:
        try:
            obj.set_editor_property(candidate, value)
            return True
        except Exception as error:
            last_error = error
    raise RuntimeError("Failed to set {} on {}: {}".format(prop_name, obj, last_error))


def enum_value(enum_type, *candidate_names):
    normalized_candidates = {
        name.lower().replace("_", "").replace(" ", "")
        for name in candidate_names
    }
    for attr_name in dir(enum_type):
        normalized_attr = attr_name.lower().replace("_", "").replace(" ", "")
        if normalized_attr in normalized_candidates:
            return getattr(enum_type, attr_name)
    raise RuntimeError("Could not find enum value {} on {}".format(candidate_names, enum_type))


def unreal_type(type_name):
    candidates = [type_name]
    if type_name.startswith("E"):
        candidates.append(type_name[1:])
    if type_name.startswith("F"):
        candidates.append(type_name[1:])
    if type_name.startswith("U"):
        candidates.append(type_name[1:])
    if type_name.startswith("A"):
        candidates.append(type_name[1:])

    for candidate in candidates:
        found = getattr(unreal, candidate, None)
        if found:
            return found

    raise RuntimeError("Could not find Unreal Python type {}. Tried: {}".format(type_name, ", ".join(candidates)))


def load_class(path):
    loaded = unreal.load_class(None, path)
    if not loaded and path.startswith("/Game/") and hasattr(unreal.EditorAssetLibrary, "load_blueprint_class"):
        asset_path = path.split(".")[0]
        try:
            loaded = unreal.EditorAssetLibrary.load_blueprint_class(asset_path)
        except Exception:
            loaded = None
    if not loaded:
        raise RuntimeError("Class not found: {}".format(path))
    return loaded


def load_struct(path):
    loaded = unreal.load_object(None, path)
    if not loaded:
        raise RuntimeError("Struct not found: {}".format(path))
    return loaded


def make_data_table(asset_name, struct_path):
    unreal.EditorAssetLibrary.make_directory(DATA_DIR)
    asset_path = "{}/{}".format(DATA_DIR, asset_name)
    row_struct = load_struct(struct_path)

    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        data_table = unreal.EditorAssetLibrary.load_asset(asset_path)
        existing_struct = None
        for prop_name in ("row_struct", "RowStruct"):
            try:
                existing_struct = data_table.get_editor_property(prop_name)
                break
            except Exception:
                pass

        if existing_struct == row_struct:
            return data_table

        warn("{} exists with a different row struct; deleting and recreating.".format(asset_path))
        unreal.EditorAssetLibrary.delete_asset(asset_path)

    factory = unreal.DataTableFactory()
    set_prop(factory, "struct", row_struct)
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    data_table = asset_tools.create_asset(asset_name, DATA_DIR, unreal.DataTable, factory)
    if not data_table:
        raise RuntimeError("Failed to create data table {}".format(asset_path))
    return data_table


def table_row_names(data_table):
    try:
        return [str(name) for name in unreal.DataTableFunctionLibrary.get_data_table_row_names(data_table)]
    except Exception:
        return []


def add_or_replace_row(data_table, row_name, row_data):
    names = table_row_names(data_table)
    if row_name in names and hasattr(unreal.DataTableFunctionLibrary, "remove_data_table_row"):
        unreal.DataTableFunctionLibrary.remove_data_table_row(data_table, row_name)

    if not hasattr(unreal.DataTableFunctionLibrary, "add_data_table_row"):
        raise RuntimeError("DataTableFunctionLibrary.add_data_table_row is unavailable in this UE build.")

    unreal.DataTableFunctionLibrary.add_data_table_row(data_table, row_name, row_data)
    unreal.EditorAssetLibrary.save_loaded_asset(data_table)


def fill_table_from_rows(data_table, rows):
    if not hasattr(unreal.DataTableFunctionLibrary, "fill_data_table_from_json_string"):
        raise RuntimeError("DataTableFunctionLibrary.fill_data_table_from_json_string is unavailable in this UE build.")

    payload_rows = []
    for row_name, row in rows.items():
        payload_row = {"Name": row_name}
        payload_row.update(row)
        payload_rows.append(payload_row)

    payload = json.dumps(payload_rows, ensure_ascii=False, indent=2)
    result = unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(data_table, payload)
    if result is False:
        raise RuntimeError("Failed to import JSON into {}".format(data_table.get_path_name()))

    if isinstance(result, (list, tuple)) and len(result) > 0:
        warn("JSON import returned messages for {}: {}".format(data_table.get_path_name(), result))

    actual_names = set(table_row_names(data_table))
    missing_names = sorted(set(rows.keys()) - actual_names)
    if missing_names:
        raise RuntimeError("Failed to import rows {} into {}".format(missing_names, data_table.get_path_name()))

    unreal.EditorAssetLibrary.save_loaded_asset(data_table)


def make_linear_color(color_tuple):
    return unreal.LinearColor(color_tuple[0], color_tuple[1], color_tuple[2], color_tuple[3])


def material_property(*candidate_names):
    return enum_value(unreal.MaterialProperty, *candidate_names)


def make_material(asset_name, color_tuple):
    unreal.EditorAssetLibrary.make_directory(MATERIAL_DIR)
    asset_path = "{}/{}".format(MATERIAL_DIR, asset_name)

    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        material = unreal.EditorAssetLibrary.load_asset(asset_path)
    else:
        factory = unreal.MaterialFactoryNew()
        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        material = asset_tools.create_asset(asset_name, MATERIAL_DIR, unreal.Material, factory)
        if not material:
            raise RuntimeError("Failed to create material {}".format(asset_path))

    color = make_linear_color(color_tuple)
    try:
        set_prop(material, "DiffuseColor", color)
    except Exception:
        pass

    if hasattr(unreal, "MaterialEditingLibrary"):
        library = unreal.MaterialEditingLibrary
        try:
            if hasattr(library, "delete_all_material_expressions"):
                library.delete_all_material_expressions(material)

            color_expression = library.create_material_expression(material, unreal.MaterialExpressionConstant3Vector, -400, 0)
            set_prop(color_expression, "Constant", color)
            library.connect_material_property(
                color_expression,
                "",
                material_property("MP_BaseColor", "BaseColor", "MP_BASE_COLOR"),
            )

            roughness_expression = library.create_material_expression(material, unreal.MaterialExpressionConstant, -400, 160)
            set_prop(roughness_expression, "R", 0.85)
            library.connect_material_property(
                roughness_expression,
                "",
                material_property("MP_Roughness", "Roughness", "MP_ROUGHNESS"),
            )

            if hasattr(library, "recompile_material"):
                library.recompile_material(material)
        except Exception as error:
            warn("Could not fully rebuild material graph for {}: {}".format(asset_path, error))

    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


def make_materials():
    materials = {}
    for key, spec in MATERIAL_SPECS.items():
        materials[key] = make_material(spec[0], spec[1])
    log("Materials created/refreshed under {}".format(MATERIAL_DIR))
    return materials


def blueprint_asset_path(asset_name):
    return "{}/{}".format(BLUEPRINT_DIR, asset_name)


def blueprint_class_path(asset_name):
    return "{0}/{1}.{1}_C".format(BLUEPRINT_DIR, asset_name)


def get_blueprint_parent_class(blueprint):
    try:
        return blueprint.get_editor_property("parent_class")
    except Exception:
        return None


def compile_blueprint(blueprint):
    if hasattr(unreal, "KismetEditorUtilities"):
        unreal.KismetEditorUtilities.compile_blueprint(blueprint)
    elif hasattr(unreal, "BlueprintEditorLibrary"):
        unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)


def get_blueprint_generated_class(blueprint):
    asset_path = blueprint.get_path_name().split(".")[0]
    if hasattr(unreal.EditorAssetLibrary, "load_blueprint_class"):
        try:
            generated_class = unreal.EditorAssetLibrary.load_blueprint_class(asset_path)
            if generated_class:
                return generated_class
        except Exception:
            pass

    for prop_name in ("generated_class", "GeneratedClass"):
        try:
            generated_class = blueprint.get_editor_property(prop_name)
            if generated_class:
                return generated_class
        except Exception:
            pass
    compile_blueprint(blueprint)
    for prop_name in ("generated_class", "GeneratedClass"):
        try:
            generated_class = blueprint.get_editor_property(prop_name)
            if generated_class:
                return generated_class
        except Exception:
            pass

    class_path = "{}.{}_C".format(asset_path, asset_path.rsplit("/", 1)[-1])
    try:
        generated_class = unreal.load_class(None, class_path)
        if generated_class:
            return generated_class
    except Exception:
        pass

    raise RuntimeError("Generated class not found for {}".format(blueprint.get_path_name()))


def get_class_default_object(generated_class):
    if hasattr(unreal, "get_default_object"):
        return unreal.get_default_object(generated_class)
    if hasattr(generated_class, "get_default_object"):
        return generated_class.get_default_object()
    raise RuntimeError("Default object not found for {}".format(generated_class))


def make_blueprint(asset_name, parent_class_path):
    unreal.EditorAssetLibrary.make_directory(BLUEPRINT_DIR)
    asset_path = blueprint_asset_path(asset_name)
    parent_class = load_class(parent_class_path)

    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        blueprint = unreal.EditorAssetLibrary.load_asset(asset_path)
        existing_parent = get_blueprint_parent_class(blueprint)
        if existing_parent and existing_parent != parent_class:
            warn("{} exists with a different parent; deleting and recreating.".format(asset_path))
            unreal.EditorAssetLibrary.delete_asset(asset_path)
            blueprint = None
    else:
        blueprint = None

    if not blueprint:
        factory = unreal.BlueprintFactory()
        set_prop(factory, "ParentClass", parent_class)
        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        blueprint = asset_tools.create_asset(asset_name, BLUEPRINT_DIR, unreal.Blueprint, factory)
        if not blueprint:
            raise RuntimeError("Failed to create blueprint {}".format(asset_path))

    return blueprint


def apply_static_mesh_defaults(class_default_object, mesh_path, material):
    mesh_component = get_static_mesh_component(class_default_object)
    mesh = load_asset(mesh_path)
    try:
        mesh_component.set_static_mesh(mesh)
    except Exception:
        set_prop(mesh_component, "StaticMesh", mesh)
    mesh_component.set_material(0, material)
    disable_collision(mesh_component)


def apply_visual_material_default(class_default_object, material):
    set_prop(class_default_object, "VisualMaterial", material)


def first_component_by_class(actor, component_type):
    if not component_type:
        return None

    try:
        components = actor.get_components_by_class(component_type)
        if components:
            return components[0]
    except Exception:
        pass
    return None


def apply_camera_defaults(class_default_object, field_of_view):
    camera_component = first_component_by_class(class_default_object, getattr(unreal, "CameraComponent", None))
    if not camera_component:
        return

    try:
        camera_component.set_field_of_view(field_of_view)
        return
    except Exception:
        pass

    try:
        set_prop(camera_component, "FieldOfView", field_of_view)
    except Exception:
        pass


def make_blueprints(materials):
    blueprints = {}
    class_paths = {}

    for key, spec in BLUEPRINT_SPECS.items():
        blueprint = make_blueprint(spec["asset_name"], spec["parent"])
        generated_class = get_blueprint_generated_class(blueprint)
        cdo = get_class_default_object(generated_class)

        if spec["parent"] == "/Script/Engine.StaticMeshActor":
            material = materials[spec["material"]]
            apply_static_mesh_defaults(cdo, spec["mesh"], material)
        elif spec.get("material"):
            material = materials[spec["material"]]
            apply_visual_material_default(cdo, material)
        elif spec["parent"] == "/Script/Engine.CameraActor":
            apply_camera_defaults(cdo, 55.0)

        compile_blueprint(blueprint)
        unreal.EditorAssetLibrary.save_loaded_asset(blueprint)
        blueprints[key] = blueprint
        class_paths[key] = blueprint_class_path(spec["asset_name"])

    log("Blueprints created/refreshed under {}".format(BLUEPRINT_DIR))
    return {"blueprints": blueprints, "class_paths": class_paths}


def build_manual_validation_assets():
    materials = make_materials()
    blueprints = make_blueprints(materials)
    return {"materials": materials, "class_paths": blueprints["class_paths"]}


def set_soft_class(row, prop_name, class_path):
    loaded_class = load_class(class_path)
    values_to_try = [loaded_class, class_path]
    if hasattr(unreal, "SoftClassPath"):
        values_to_try.append(unreal.SoftClassPath(class_path))
    if hasattr(unreal, "SoftObjectPath"):
        values_to_try.append(unreal.SoftObjectPath(class_path))

    last_error = None
    for value in values_to_try:
        try:
            set_prop(row, prop_name, value)
            return
        except Exception as error:
            last_error = error
    raise RuntimeError("Failed to set soft class {}: {}".format(prop_name, last_error))


def make_name_stack(item_id, count):
    stack = unreal.TTDNameStack()
    set_prop(stack, "Id", item_id)
    set_prop(stack, "Count", count)
    return stack


def make_runtime_stats(max_health, damage, attack_range, interval, projectile_speed, move_speed=0.0):
    stats = unreal.TTDBattleBuildingRuntimeStats()
    set_prop(stats, "MaxHealth", max_health)
    set_prop(stats, "AttackDamage", damage)
    set_prop(stats, "AttackRange", attack_range)
    set_prop(stats, "AttackInterval", interval)
    set_prop(stats, "ProjectileSpeed", projectile_speed)
    set_prop(stats, "MoveSpeed", move_speed)
    return stats


def build_data_tables(asset_context):
    class_paths = asset_context["class_paths"]

    tables = {
        "research_parts": make_data_table("DT_ResearchParts_Manual", "/Script/ToyTowerDefense.TTDPartDefinition"),
        "research_diagrams": make_data_table("DT_ResearchDiagrams_Manual", "/Script/ToyTowerDefense.TTDDiagramDefinition"),
        "research_toy_boxes": make_data_table("DT_ResearchToyBoxes_Manual", "/Script/ToyTowerDefense.TTDToyBoxDefinition"),
        "levels": make_data_table("DT_BattleLevels_Manual", "/Script/ToyTowerDefense.TTDBattleLevelDefinition"),
        "waves": make_data_table("DT_BattleWaves_Manual", "/Script/ToyTowerDefense.TTDWaveDefinition"),
        "enemies": make_data_table("DT_BattleEnemies_Manual", "/Script/ToyTowerDefense.TTDEnemyDefinition"),
        "buildings": make_data_table("DT_BattleBuildings_Manual", "/Script/ToyTowerDefense.TTDBuildingDefinition"),
        "toy_boxes": make_data_table("DT_BattleToyBoxRewards_Manual", "/Script/ToyTowerDefense.TTDToyBoxRewardDefinition"),
        "height_effects": make_data_table("DT_BattleHeightEffects_Manual", "/Script/ToyTowerDefense.TTDBattleHeightEffectDefinition"),
        "object_pool": make_data_table("DT_ObjectPool_Manual", "/Script/ToyTowerDefense.TTDObjectPoolDefinition"),
    }

    fill_table_from_rows(
        tables["research_parts"],
        {
            "Gear": {"PartId": "Gear", "DisplayName": "齿轮", "Description": "基础机械零件，可驱动玩具机关。"},
            "Spring": {"PartId": "Spring", "DisplayName": "弹簧", "Description": "为塔防玩具提供弹射和回弹能力。"},
            "Battery": {"PartId": "Battery", "DisplayName": "电池", "Description": "提供持续能量的核心零件。"},
            "Wheel": {"PartId": "Wheel", "DisplayName": "车轮", "Description": "适用于移动或旋转类玩具。"},
            "Ribbon": {"PartId": "Ribbon", "DisplayName": "丝带", "Description": "装饰类零件，可用于提升玩具表现。"},
            "Star": {"PartId": "Star", "DisplayName": "星徽", "Description": "稀有装饰零件，常用于高级图纸。"},
            "Dice": {"PartId": "Dice", "DisplayName": "骰子", "Description": "带有随机主题的趣味零件。"},
            "Card": {"PartId": "Card", "DisplayName": "纸牌", "Description": "用于扑克主题玩具的基础材料。"},
        },
    )

    fill_table_from_rows(
        tables["research_diagrams"],
        {
            "BasicTower": {
                "DiagramId": "BasicTower",
                "DisplayName": "基础塔图纸",
                "Description": "基础攻击塔图纸，需要齿轮与弹簧搭建。",
                "RequiredPartIds": ["Gear", "Spring"],
                "PartCosts": [{"Id": "Gear", "Count": 1}, {"Id": "Spring", "Count": 1}],
            },
            "ProjectileTower": {
                "DiagramId": "ProjectileTower",
                "DisplayName": "弹射塔图纸",
                "Description": "投射物塔图纸，需要齿轮与弹簧搭建。",
                "RequiredPartIds": ["Gear", "Spring"],
                "PartCosts": [{"Id": "Gear", "Count": 1}, {"Id": "Spring", "Count": 1}],
            },
            "JokerTower": {
                "DiagramId": "JokerTower",
                "DisplayName": "小丑塔图纸",
                "Description": "基础攻击塔图纸，需要齿轮与弹簧搭建。",
                "RequiredPartIds": ["Gear", "Spring"],
                "PartCosts": [{"Id": "Gear", "Count": 1}, {"Id": "Spring", "Count": 1}],
            },
            "BatteryBlaster": {
                "DiagramId": "BatteryBlaster",
                "DisplayName": "电池炮台图纸",
                "Description": "能量型炮台图纸，需要电池和齿轮。",
                "RequiredPartIds": ["Battery", "Gear"],
                "PartCosts": [{"Id": "Battery", "Count": 1}, {"Id": "Gear", "Count": 1}],
            },
            "RollingGuard": {
                "DiagramId": "RollingGuard",
                "DisplayName": "滚轮守卫图纸",
                "Description": "控制路径的玩具守卫，需要车轮和弹簧。",
                "RequiredPartIds": ["Wheel", "Spring"],
                "PartCosts": [{"Id": "Wheel", "Count": 1}, {"Id": "Spring", "Count": 1}],
            },
            "RibbonTrap": {
                "DiagramId": "RibbonTrap",
                "DisplayName": "丝带陷阱图纸",
                "Description": "减速类陷阱图纸，需要丝带和齿轮。",
                "RequiredPartIds": ["Ribbon", "Gear"],
                "PartCosts": [{"Id": "Ribbon", "Count": 1}, {"Id": "Gear", "Count": 1}],
            },
            "StarBeacon": {
                "DiagramId": "StarBeacon",
                "DisplayName": "星徽信标图纸",
                "Description": "高级辅助塔图纸，需要星徽、电池和纸牌。",
                "RequiredPartIds": ["Star", "Battery", "Card"],
                "PartCosts": [{"Id": "Star", "Count": 1}, {"Id": "Battery", "Count": 1}, {"Id": "Card", "Count": 1}],
            },
            "DiceLauncher": {
                "DiagramId": "DiceLauncher",
                "DisplayName": "骰子发射器图纸",
                "Description": "随机弹道塔图纸，需要骰子和弹簧。",
                "RequiredPartIds": ["Dice", "Spring"],
                "PartCosts": [{"Id": "Dice", "Count": 1}, {"Id": "Spring", "Count": 1}],
            },
            "CardWall": {
                "DiagramId": "CardWall",
                "DisplayName": "纸牌墙图纸",
                "Description": "防御障碍图纸，需要纸牌和丝带。",
                "RequiredPartIds": ["Card", "Ribbon"],
                "PartCosts": [{"Id": "Card", "Count": 1}, {"Id": "Ribbon", "Count": 1}],
            },
            "ClockworkToy": {
                "DiagramId": "ClockworkToy",
                "DisplayName": "发条玩具图纸",
                "Description": "复合型玩具图纸，需要齿轮、弹簧和车轮。",
                "RequiredPartIds": ["Gear", "Spring", "Wheel"],
                "PartCosts": [{"Id": "Gear", "Count": 1}, {"Id": "Spring", "Count": 1}, {"Id": "Wheel", "Count": 1}],
            },
            "PrismJoker": {
                "DiagramId": "PrismJoker",
                "DisplayName": "棱彩小丑图纸",
                "Description": "稀有攻击塔图纸，需要星徽和骰子。",
                "RequiredPartIds": ["Star", "Dice"],
                "PartCosts": [{"Id": "Star", "Count": 1}, {"Id": "Dice", "Count": 1}],
            },
            "CarnivalCore": {
                "DiagramId": "CarnivalCore",
                "DisplayName": "狂欢核心图纸",
                "Description": "终阶核心图纸，需要多种高级零件组合。",
                "RequiredPartIds": ["Star", "Battery", "Dice", "Card"],
                "PartCosts": [{"Id": "Star", "Count": 1}, {"Id": "Battery", "Count": 1}, {"Id": "Dice", "Count": 1}, {"Id": "Card", "Count": 1}],
            },
        },
    )

    fill_table_from_rows(
        tables["research_toy_boxes"],
        {
            "BasicToyBox": {
                "ToyBoxId": "BasicToyBox",
                "DisplayName": "基础玩具盒",
                "Description": "常见零件玩具盒，适合早期制作。",
                "PossiblePartIds": ["Gear", "Spring", "Card"],
                "CraftDurationSeconds": 20.0,
            },
            "MechanicToyBox": {
                "ToyBoxId": "MechanicToyBox",
                "DisplayName": "机械玩具盒",
                "Description": "偏向机械结构零件的玩具盒。",
                "PossiblePartIds": ["Gear", "Wheel", "Spring"],
                "CraftDurationSeconds": 35.0,
            },
            "EnergyToyBox": {
                "ToyBoxId": "EnergyToyBox",
                "DisplayName": "能量玩具盒",
                "Description": "更容易开出电池类零件。",
                "PossiblePartIds": ["Battery", "Gear", "Star"],
                "CraftDurationSeconds": 45.0,
            },
            "PartyToyBox": {
                "ToyBoxId": "PartyToyBox",
                "DisplayName": "派对玩具盒",
                "Description": "包含装饰和随机主题零件。",
                "PossiblePartIds": ["Ribbon", "Dice", "Card"],
                "CraftDurationSeconds": 50.0,
            },
            "RareToyBox": {
                "ToyBoxId": "RareToyBox",
                "DisplayName": "稀有玩具盒",
                "Description": "有机会获得高级图纸所需零件。",
                "PossiblePartIds": ["Star", "Battery", "Dice"],
                "CraftDurationSeconds": 70.0,
            },
            "CardToyBox": {
                "ToyBoxId": "CardToyBox",
                "DisplayName": "纸牌玩具盒",
                "Description": "纸牌主题零件集合。",
                "PossiblePartIds": ["Card", "Dice", "Ribbon"],
                "CraftDurationSeconds": 40.0,
            },
            "GuardToyBox": {
                "ToyBoxId": "GuardToyBox",
                "DisplayName": "守卫玩具盒",
                "Description": "防御类玩具常用零件集合。",
                "PossiblePartIds": ["Wheel", "Gear", "Battery"],
                "CraftDurationSeconds": 55.0,
            },
        },
    )

    fill_table_from_rows(
        tables["height_effects"],
        {
            "HighGround": {
                "HeightEffectId": "HighGround",
                "Modifiers": [
                    {"Attribute": "AttackDamage", "Operation": "Add", "Value": 5.0},
                    {"Attribute": "AttackRange", "Operation": "Multiply", "Value": 2.0},
                    {"Attribute": "AttackInterval", "Operation": "Override", "Value": 0.25},
                ],
            }
        },
    )

    fill_table_from_rows(
        tables["enemies"],
        {
            "BasicEnemy": {
                "EnemyId": "BasicEnemy",
                "DisplayName": "基础敌人",
                "EnemyClass": class_paths["enemy"],
                "MaxHealth": 25.0,
                "MoveSpeed": 90.0,
                "AttackDamage": 1.0,
                "AttackRange": 100.0,
                "AttackInterval": 1.0,
                "ProgressWeight": 1.0,
                "CurrencyDrop": 1,
                "CurrencyDropChance": 1.0,
            }
        },
    )

    fill_table_from_rows(
        tables["waves"],
        {
            "WaveA": {
                "WaveId": "WaveA",
                "DelayBeforeWave": 0.0,
                "WaveDurationSeconds": 30.0,
                "EnemyEntries": [
                    {
                        "EnemyId": "BasicEnemy",
                        "Count": 40,
                        "SpawnInterval": 0.75,
                        "SpawnGroup": "Edge",
                    }
                ],
            }
        },
    )

    fill_table_from_rows(
        tables["buildings"],
        {
            "BasicTower": {
                "BuildingId": "BasicTower",
                "DisplayName": "基础塔",
                "RequiredDiagramId": "BasicTower",
                "BuildingClass": class_paths["basic_tower"],
                "PartCosts": [{"Id": "Gear", "Count": 2}],
                "AttackMode": "InstantDamage",
                "BaseStats": {
                    "MaxHealth": 50.0,
                    "AttackDamage": 5.0,
                    "AttackRange": 1000.0,
                    "AttackInterval": 0.5,
                    "ProjectileSpeed": 900.0,
                    "MoveSpeed": 0.0,
                },
                "ProjectileClass": "None",
            },
            "ProjectileTower": {
                "BuildingId": "ProjectileTower",
                "DisplayName": "弹射塔",
                "RequiredDiagramId": "ProjectileTower",
                "BuildingClass": class_paths["projectile_tower"],
                "PartCosts": [{"Id": "Gear", "Count": 1}, {"Id": "Spring", "Count": 1}],
                "AttackMode": "Projectile",
                "BaseStats": {
                    "MaxHealth": 50.0,
                    "AttackDamage": 10.0,
                    "AttackRange": 1000.0,
                    "AttackInterval": 0.35,
                    "ProjectileSpeed": 3000.0,
                    "MoveSpeed": 0.0,
                },
                "ProjectileClass": class_paths["projectile"],
            },
        },
    )

    fill_table_from_rows(
        tables["toy_boxes"],
        {
            "BasicToyBox": {
                "ToyBoxId": "BasicToyBox",
                "DisplayName": "基础玩具盒",
                "PurchaseCost": 5,
                "RollCount": 2,
                "RewardEntries": [
                    {
                        "PartId": "Gear",
                        "Weight": 1.0,
                        "MinQuantity": 1,
                        "MaxQuantity": 1,
                    }
                ],
            },
            "MechanicToyBox": {
                "ToyBoxId": "MechanicToyBox",
                "DisplayName": "机械玩具盒",
                "PurchaseCost": 8,
                "RollCount": 2,
                "RewardEntries": [
                    {
                        "PartId": "Wheel",
                        "Weight": 1.0,
                        "MinQuantity": 1,
                        "MaxQuantity": 1,
                    }
                ],
            },
            "EnergyToyBox": {
                "ToyBoxId": "EnergyToyBox",
                "DisplayName": "能量玩具盒",
                "PurchaseCost": 8,
                "RollCount": 2,
                "RewardEntries": [
                    {
                        "PartId": "Battery",
                        "Weight": 1.0,
                        "MinQuantity": 1,
                        "MaxQuantity": 1,
                    }
                ],
            },
        },
    )

    fill_table_from_rows(
        tables["levels"],
        {
            DEFAULT_LEVEL_ID: {
                "LevelId": DEFAULT_LEVEL_ID,
                "DisplayName": "原型战斗",
                "Description": "用于最小手动验证的玩具塔防原型关卡。",
                "Background": {
                    "bSpawnBackground": True,
                    "BackgroundClass": "/Script/ToyTowerDefense.TTDBattleBackgroundActor",
                    "Transform": {
                        "Rotation": {"X": 0.0, "Y": 0.0, "Z": 0.0, "W": 1.0},
                        "Translation": {"X": 0.0, "Y": 0.0, "Z": -20.0},
                        "Scale3D": {"X": 1.0, "Y": 1.0, "Z": 1.0},
                    },
                    "ArenaHalfExtent": {"X": 1200.0, "Y": 1200.0},
                    "GroundThickness": 24.0,
                    "WallHeight": 260.0,
                    "WallThickness": 80.0,
                    "GroundColor": {"R": 0.38, "G": 0.58, "B": 0.42, "A": 1.0},
                    "WallColor": {"R": 0.56, "G": 0.76, "B": 0.86, "A": 1.0},
                    "LightColor": {"R": 1.0, "G": 0.93, "B": 0.82, "A": 1.0},
                },
                "CastleClass": class_paths["castle"],
                "CastleTransform": {
                    "Rotation": {"X": 0.0, "Y": 0.0, "Z": 0.0, "W": 1.0},
                    "Translation": {"X": 0.0, "Y": 0.0, "Z": 0.0},
                    "Scale3D": {"X": 1.0, "Y": 1.0, "Z": 1.0},
                },
                "CastleMaxHealth": 500.0,
                "WaveIds": ["WaveA"],
                "StartingDiagramIds": ["BasicTower", "ProjectileTower"],
                "StartingToyBoxes": [{"Id": "BasicToyBox", "Count": 1}],
                "StartingParts": [{"Id": "Gear", "Count": 20}, {"Id": "Spring", "Count": 10}],
                "StartingCurrency": 10,
                "MaxSelectedDiagrams": 3,
                "MaxSelectedToyBoxes": 3,
                "PreparationDurationSeconds": 3.0,
                "VictoryReturnDelaySeconds": 5.0,
            }
        },
    )

    fill_table_from_rows(
        tables["object_pool"],
        {
            "Enemy": {
                "ObjectClass": class_paths["enemy"],
                "InitialSize": 0,
                "MaxSize": 64,
                "ExpandBy": 4,
                "bPrewarmOnWorldBeginPlay": True,
            },
            "BasicTower": {
                "ObjectClass": class_paths["basic_tower"],
                "InitialSize": 0,
                "MaxSize": 16,
                "ExpandBy": 4,
                "bPrewarmOnWorldBeginPlay": True,
            },
            "ProjectileTower": {
                "ObjectClass": class_paths["projectile_tower"],
                "InitialSize": 0,
                "MaxSize": 16,
                "ExpandBy": 4,
                "bPrewarmOnWorldBeginPlay": True,
            },
            "Projectile": {
                "ObjectClass": class_paths["projectile"],
                "InitialSize": 0,
                "MaxSize": 32,
                "ExpandBy": 8,
                "bPrewarmOnWorldBeginPlay": True,
            },
        },
    )

    log("DataTables created/refreshed under {}".format(DATA_DIR))
    return tables


def gameplay_tag_to_string(tag):
    for prop_name in ("TagName", "tag_name"):
        try:
            value = tag.get_editor_property(prop_name)
            value_text = str(value)
            if value_text and value_text.lower() != "none":
                return value_text
        except Exception:
            pass

    text = str(tag)
    if "TagName=\"" in text:
        return text.split("TagName=\"", 1)[1].split("\"", 1)[0]
    if "tag_name=\"" in text:
        return text.split("tag_name=\"", 1)[1].split("\"", 1)[0]
    if "'" in text:
        pieces = text.split("'")
        for piece in pieces:
            if piece.startswith("TTD."):
                return piece
    return text


def make_gameplay_tag(tag_name):
    name_values = [tag_name]
    if hasattr(unreal, "Name"):
        name_values.append(unreal.Name(tag_name))

    manager_class = getattr(unreal, "GameplayTagsManager", None)
    if manager_class:
        managers = [manager_class]
        for getter_name in ("get", "get_manager", "Get"):
            getter = getattr(manager_class, getter_name, None)
            if getter:
                try:
                    managers.insert(0, getter())
                except Exception:
                    pass

        for manager in managers:
            for function_name in ("request_gameplay_tag", "RequestGameplayTag"):
                function = getattr(manager, function_name, None)
                if not function:
                    continue
                for name_value in name_values:
                    for args in ((name_value, False), (name_value, True), (name_value,)):
                        try:
                            return function(*args)
                        except Exception:
                            pass

    for owner_name in ("GameplayTag", "GameplayTagsManager"):
        owner = getattr(unreal, owner_name, None)
        if not owner:
            continue
        for function_name in ("request_gameplay_tag", "RequestGameplayTag"):
            function = getattr(owner, function_name, None)
            if function:
                try:
                    return function(tag_name, False)
                except TypeError:
                    try:
                        return function(tag_name)
                    except Exception:
                        pass
                except Exception:
                    pass

    raise RuntimeError("Could not request gameplay tag {}".format(tag_name))


def get_default_object(class_type):
    if hasattr(unreal, "get_mutable_default"):
        return unreal.get_mutable_default(class_type)
    if hasattr(unreal, "get_default_object"):
        return unreal.get_default_object(class_type)
    if hasattr(class_type, "get_default_object"):
        return class_type.get_default_object()
    raise RuntimeError("Could not get default object for {}".format(class_type))


def update_ini_section(config_path, section_name, replacement_by_prefix, append_lines):
    existing_lines = []
    if os.path.exists(config_path):
        with open(config_path, "r", encoding="utf-8") as config_file:
            existing_lines = config_file.read().splitlines()

    section_header = "[{}]".format(section_name)
    output_lines = []
    index = 0
    found_section = False

    while index < len(existing_lines):
        line = existing_lines[index]
        if line.strip() != section_header:
            output_lines.append(line)
            index += 1
            continue

        found_section = True
        output_lines.append(line)
        index += 1

        while index < len(existing_lines) and not existing_lines[index].lstrip().startswith("["):
            section_line = existing_lines[index]
            stripped_line = section_line.strip()
            if not any(stripped_line.startswith(prefix) for prefix in replacement_by_prefix):
                output_lines.append(section_line)
            index += 1

        output_lines.extend(append_lines)
        continue

    if not found_section:
        if output_lines and output_lines[-1].strip():
            output_lines.append("")
        output_lines.append(section_header)
        output_lines.extend(append_lines)

    with open(config_path, "w", encoding="utf-8", newline="\n") as config_file:
        config_file.write("\n".join(output_lines).rstrip() + "\n")


def data_table_config_path(data_table):
    return data_table.get_path_name()


def gameplay_tag_map_entry(tag_name, data_table):
    value_text = data_table_config_path(data_table) if data_table else "None"
    return '((TagName="{}"), {})'.format(tag_name, value_text)


def write_developer_settings_config(tag_to_table):
    config_path = os.path.normpath(os.path.join(unreal.Paths.project_config_dir(), "DefaultGame.ini"))
    all_tags = [
        "TTD.DataTable.Research.Parts",
        "TTD.DataTable.Research.Diagrams",
        "TTD.DataTable.Research.ToyBoxes",
        "TTD.DataTable.ObjectPool.Definitions",
        "TTD.DataTable.Battle.Levels",
        "TTD.DataTable.Battle.Waves",
        "TTD.DataTable.Battle.Enemies",
        "TTD.DataTable.Battle.Buildings",
        "TTD.DataTable.Battle.ToyBoxRewards",
        "TTD.DataTable.Battle.HeightEffects",
    ]
    map_entries = [gameplay_tag_map_entry(tag_name, tag_to_table.get(tag_name)) for tag_name in all_tags]

    update_ini_section(
        config_path,
        "/Script/ToyTowerDefense.TTDDeveloperSettings",
        ["DefaultBattleLevelId=", "GameplayTagDataTables=", "+GameplayTagDataTables=", "!GameplayTagDataTables="],
        [
            "DefaultBattleLevelId={}".format(DEFAULT_LEVEL_ID),
            "GameplayTagDataTables=({})".format(",".join(map_entries)),
        ],
    )
    log("DefaultGame.ini updated for DeveloperSettings.")


def write_game_maps_settings_config():
    config_path = os.path.normpath(os.path.join(unreal.Paths.project_config_dir(), "DefaultEngine.ini"))
    update_ini_section(
        config_path,
        "/Script/EngineSettings.GameMapsSettings",
        ["GameDefaultMap=", "EditorStartupMap="],
        [
            "GameDefaultMap={}".format(VALIDATION_MAP_ASSET),
            "EditorStartupMap={}".format(VALIDATION_MAP_ASSET),
        ],
    )
    log("DefaultEngine.ini updated for validation map.")


def configure_developer_settings(tables):
    settings = get_default_object(unreal.TTDDeveloperSettings)
    set_prop(settings, "DefaultBattleLevelId", DEFAULT_LEVEL_ID)

    tag_to_table = {
        "TTD.DataTable.Research.Parts": tables["research_parts"],
        "TTD.DataTable.Research.Diagrams": tables["research_diagrams"],
        "TTD.DataTable.Research.ToyBoxes": tables["research_toy_boxes"],
        "TTD.DataTable.Battle.Levels": tables["levels"],
        "TTD.DataTable.Battle.Waves": tables["waves"],
        "TTD.DataTable.Battle.Enemies": tables["enemies"],
        "TTD.DataTable.Battle.Buildings": tables["buildings"],
        "TTD.DataTable.Battle.ToyBoxRewards": tables["toy_boxes"],
        "TTD.DataTable.Battle.HeightEffects": tables["height_effects"],
        "TTD.DataTable.ObjectPool.Definitions": tables["object_pool"],
    }

    current_map = settings.get_editor_property("gameplay_tag_data_tables")
    key_by_name = {gameplay_tag_to_string(key): key for key in current_map.keys()}

    def apply_map(value_factory):
        for tag_name, data_table in tag_to_table.items():
            key = key_by_name.get(tag_name) or make_gameplay_tag(tag_name)
            current_map[key] = value_factory(data_table)

    try:
        apply_map(lambda table: table)
    except Exception as error:
        warn("Could not update DeveloperSettings in memory: {}".format(error))

    write_developer_settings_config(tag_to_table)
    log("DeveloperSettings configured. DefaultBattleLevelId={}".format(DEFAULT_LEVEL_ID))


def get_actor_subsystem():
    if hasattr(unreal, "EditorActorSubsystem"):
        return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    return None


def all_level_actors():
    subsystem = get_actor_subsystem()
    if subsystem:
        return subsystem.get_all_level_actors()
    return unreal.EditorLevelLibrary.get_all_level_actors()


def actor_by_label(label):
    for actor in all_level_actors():
        if actor.get_actor_label() == label:
            return actor
    return None


def destroy_actor(actor):
    subsystem = get_actor_subsystem()
    if subsystem:
        try:
            return subsystem.destroy_actor(actor)
        except Exception:
            pass
    return unreal.EditorLevelLibrary.destroy_actor(actor)


def clear_manual_validation_actors():
    actors_to_delete = []
    for actor in all_level_actors():
        try:
            if actor.get_actor_label().startswith(VALIDATION_ACTOR_PREFIX):
                actors_to_delete.append(actor)
        except Exception:
            pass

    for actor in actors_to_delete:
        destroy_actor(actor)

    if actors_to_delete:
        log("Removed {} old validation actors.".format(len(actors_to_delete)))


def clear_default_visual_environment_actors():
    removable_labels = {
        "Floor",
        "SM_SkySphere",
        "SkySphere",
        "BP_Sky_Sphere",
        "ExponentialHeightFog",
        "VolumetricCloud",
    }
    removable_class_tokens = (
        "ExponentialHeightFog",
        "VolumetricCloud",
    )

    actors_to_delete = []
    for actor in all_level_actors():
        try:
            label = actor.get_actor_label()
            class_name = actor.get_class().get_name()
            if label in removable_labels or any(token in class_name for token in removable_class_tokens):
                actors_to_delete.append(actor)
        except Exception:
            pass

    for actor in actors_to_delete:
        destroy_actor(actor)

    if actors_to_delete:
        log("Removed {} default fog/visual environment actors.".format(len(actors_to_delete)))


def spawn_actor(label, class_path, location, rotation=(0.0, 0.0, 0.0)):
    vector = unreal.Vector(location[0], location[1], location[2])
    rotator = unreal.Rotator(rotation[0], rotation[1], rotation[2])

    existing = actor_by_label(label)
    if existing:
        existing.set_actor_location(vector, False, False)
        existing.set_actor_rotation(rotator, False)
        return existing

    actor_class = load_class(class_path)

    actor = None
    subsystem = get_actor_subsystem()
    if subsystem:
        actor = subsystem.spawn_actor_from_class(actor_class, vector, rotator)
    if not actor and hasattr(unreal, "EditorLevelLibrary"):
        actor = unreal.EditorLevelLibrary.spawn_actor_from_class(actor_class, vector, rotator)

    if not actor:
        raise RuntimeError("Failed to spawn {}".format(label))
    actor.set_actor_label(label)
    actor.set_actor_location(vector, False, False)
    actor.set_actor_rotation(rotator, False)
    return actor


def set_actor_tags(actor, tags):
    name_values = []
    for tag in tags:
        try:
            name_values.append(unreal.Name(tag))
        except Exception:
            name_values.append(tag)
    try:
        set_prop(actor, "Tags", name_values)
    except Exception:
        pass


def configure_light_component(actor, component_type_name, intensity):
    component_type = getattr(unreal, component_type_name, None)
    component = first_component_by_class(actor, component_type)
    if not component:
        return

    try:
        set_prop(component, "Intensity", intensity)
    except Exception:
        pass


def configure_camera_actor(actor, field_of_view):
    set_actor_tags(actor, ["TTD.BattleCamera"])
    component_type = getattr(unreal, "CameraComponent", None)
    component = first_component_by_class(actor, component_type)
    if not component:
        return

    try:
        component.set_field_of_view(field_of_view)
        return
    except Exception:
        pass

    try:
        set_prop(component, "FieldOfView", field_of_view)
    except Exception:
        pass


def load_asset(path):
    loaded = unreal.load_object(None, path)
    if not loaded:
        raise RuntimeError("Asset not found: {}".format(path))
    return loaded


def get_static_mesh_component(actor):
    for prop_name in ("static_mesh_component", "StaticMeshComponent"):
        try:
            component = actor.get_editor_property(prop_name)
            if component:
                return component
        except Exception:
            pass

    try:
        components = actor.get_components_by_class(unreal.StaticMeshComponent)
        if components:
            return components[0]
    except Exception:
        pass

    raise RuntimeError("StaticMeshComponent not found on {}".format(actor.get_actor_label()))


def disable_collision(component):
    try:
        component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        return
    except Exception:
        pass

    try:
        component.set_editor_property("collision_enabled", unreal.CollisionEnabled.NO_COLLISION)
    except Exception:
        pass


def spawn_static_mesh_actor(label, class_path, location, scale):
    actor = spawn_actor(label, class_path, location)
    actor.set_actor_scale3d(unreal.Vector(scale[0], scale[1], scale[2]))
    return actor


def make_int_point(x, y):
    if hasattr(unreal, "IntPoint"):
        return unreal.IntPoint(x, y)
    if hasattr(unreal, "Int32Point"):
        return unreal.Int32Point(x, y)
    return (x, y)


def configure_spawn_point(actor, spawn_group):
    try:
        actor.configure_spawn_point(spawn_group)
        return
    except Exception:
        set_prop(actor, "SpawnGroup", spawn_group)


def configure_build_slot(actor, slot_id, grid_x, grid_y, height_level, height_effect_id):
    try:
        actor.configure_slot(slot_id, make_int_point(grid_x, grid_y), height_level, height_effect_id)
        return
    except Exception:
        set_prop(actor, "SlotId", slot_id)
        set_prop(actor, "GridCoordinate", make_int_point(grid_x, grid_y))
        set_prop(actor, "HeightLevel", height_level)
        set_prop(actor, "HeightEffectId", height_effect_id)


def is_null_rhi_session():
    try:
        return "-nullrhi" in unreal.SystemLibrary.get_command_line().lower()
    except Exception:
        return False


def configure_map(asset_context):
    if is_null_rhi_session():
        warn("NullRHI session detected; skipping map actor placement to avoid editor viewport hit-proxy crash. Run this script in the full Editor to place SpawnPoint/BuildSlot actors.")
        write_game_maps_settings_config()
        return

    if hasattr(unreal, "EditorLoadingAndSavingUtils"):
        unreal.EditorLoadingAndSavingUtils.load_map(VALIDATION_MAP)

    clear_manual_validation_actors()
    clear_default_visual_environment_actors()
    class_paths = asset_context["class_paths"]

    sun = spawn_actor("{}SunLight".format(VALIDATION_ACTOR_PREFIX), "/Script/Engine.DirectionalLight", (0.0, 0.0, 900.0), (-55.0, -35.0, 0.0))
    configure_light_component(sun, "DirectionalLightComponent", 4.5)

    sky_light = spawn_actor("{}SkyLight".format(VALIDATION_ACTOR_PREFIX), "/Script/Engine.SkyLight", (0.0, 0.0, 500.0))
    configure_light_component(sky_light, "SkyLightComponent", 1.0)

    camera = spawn_actor("{}BattleCamera".format(VALIDATION_ACTOR_PREFIX), class_paths["battle_camera"], (-1600.0, -1600.0, 2300.0), (-45.0, 45.0, 0.0))
    configure_camera_actor(camera, 55.0)

    spawn_static_mesh_actor("{}Ground".format(VALIDATION_ACTOR_PREFIX), class_paths["map_ground"], (0.0, 0.0, -5.0), (22.0, 22.0, 0.05))
    spawn_static_mesh_actor("{}Path_NorthSouth".format(VALIDATION_ACTOR_PREFIX), class_paths["map_path"], (0.0, 0.0, -1.0), (2.2, 18.0, 0.03))
    spawn_static_mesh_actor("{}Path_EastWest".format(VALIDATION_ACTOR_PREFIX), class_paths["map_path"], (0.0, 0.0, -0.5), (18.0, 2.2, 0.03))

    spawn_static_mesh_actor("{}Wall_North".format(VALIDATION_ACTOR_PREFIX), class_paths["map_wall"], (0.0, 1100.0, 50.0), (22.0, 0.25, 1.0))
    spawn_static_mesh_actor("{}Wall_South".format(VALIDATION_ACTOR_PREFIX), class_paths["map_wall"], (0.0, -1100.0, 50.0), (22.0, 0.25, 1.0))
    spawn_static_mesh_actor("{}Wall_East".format(VALIDATION_ACTOR_PREFIX), class_paths["map_wall"], (1100.0, 0.0, 50.0), (0.25, 22.0, 1.0))
    spawn_static_mesh_actor("{}Wall_West".format(VALIDATION_ACTOR_PREFIX), class_paths["map_wall"], (-1100.0, 0.0, 50.0), (0.25, 22.0, 1.0))

    for label, location in {
        "Decor_NorthWest": (-850.0, 850.0, 25.0),
        "Decor_NorthEast": (850.0, 850.0, 25.0),
        "Decor_SouthWest": (-850.0, -850.0, 25.0),
        "Decor_SouthEast": (850.0, -850.0, 25.0),
    }.items():
        spawn_static_mesh_actor("{}{}".format(VALIDATION_ACTOR_PREFIX, label), class_paths["map_decor"], location, (2.2, 1.2, 0.5))

    for label, location in {
        "Spawn_North": (0.0, 900.0, 0.0),
        "Spawn_South": (0.0, -900.0, 0.0),
        "Spawn_East": (900.0, 0.0, 0.0),
        "Spawn_West": (-900.0, 0.0, 0.0),
    }.items():
        spawn = spawn_actor("{}{}".format(VALIDATION_ACTOR_PREFIX, label), class_paths["spawn_point"], location)
        configure_spawn_point(spawn, "Edge")

    build_slots = [
        ("Slot_NW", (-250.0, 250.0, 0.0), -1, 1, 0, ""),
        ("Slot_NE", (250.0, 250.0, 0.0), 1, 1, 0, ""),
        ("Slot_W", (-380.0, 0.0, 0.0), -2, 0, 0, ""),
        ("Slot_E", (380.0, 0.0, 0.0), 2, 0, 0, ""),
        ("Slot_SW", (-250.0, -250.0, 0.0), -1, -1, 0, ""),
        ("Slot_SE", (250.0, -250.0, 0.0), 1, -1, 0, ""),
        ("High_NW", (-550.0, 420.0, 0.0), -3, 2, 5, "HighGround"),
        ("High_NE", (550.0, 420.0, 0.0), 3, 2, 5, "HighGround"),
        ("High_SW", (-550.0, -420.0, 0.0), -3, -2, 5, "HighGround"),
        ("High_SE", (550.0, -420.0, 0.0), 3, -2, 5, "HighGround"),
    ]
    for slot_id, location, grid_x, grid_y, height_level, height_effect_id in build_slots:
        slot_class = class_paths["high_build_slot"] if height_effect_id else class_paths["build_slot"]
        slot = spawn_actor("{}{}".format(VALIDATION_ACTOR_PREFIX, slot_id), slot_class, location)
        configure_build_slot(slot, slot_id, grid_x, grid_y, height_level, height_effect_id)

    b_map_saved = True
    if hasattr(unreal, "EditorLevelLibrary"):
        save_result = unreal.EditorLevelLibrary.save_current_level()
        if save_result is False:
            b_map_saved = False
    if hasattr(unreal, "EditorLoadingAndSavingUtils"):
        save_result = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
        if save_result is False:
            b_map_saved = False

    if not b_map_saved:
        warn("Map save reported failure. Close other Editor instances or run this script inside the already-open Editor, then rerun it.")

    write_game_maps_settings_config()

    log("Validation map configured: {}".format(VALIDATION_MAP))


def main():
    assets = build_manual_validation_assets()
    tables = build_data_tables(assets)
    configure_developer_settings(tables)
    configure_map(assets)
    log("Done. Press Play, click Start, then Level Challenge.")


if __name__ == "__main__":
    main()
