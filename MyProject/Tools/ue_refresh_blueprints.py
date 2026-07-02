import unreal


ASSET_PATHS = [
    "/Game/Blueprints/Data/BFL_HelpfulFunctions",
    "/Game/Blueprints/AC_TraversalLogic",
    "/Game/Blueprints/AC_VisualOverrideManager",
    "/Game/Blueprints/SandboxCharacter_CMC_ABP",
    "/Game/Blueprints/BP_MyThifCatcher_Sandbox",
]


def log(msg: str) -> None:
    unreal.log(f"[ue_refresh_blueprints] {msg}")


def refresh_blueprint(bp) -> None:
    refresh_attempts = [
        (getattr(unreal, "BlueprintEditorLibrary", None), "refresh_all_nodes"),
        (getattr(unreal, "KismetEditorUtilities", None), "refresh_all_nodes"),
    ]
    for owner, fn_name in refresh_attempts:
        if owner and hasattr(owner, fn_name):
            try:
                getattr(owner, fn_name)(bp)
                log(f"Refreshed nodes using {owner.__name__}.{fn_name}: {bp.get_name()}")
                return
            except Exception as exc:
                log(f"Refresh failed via {owner.__name__}.{fn_name} on {bp.get_name()}: {exc}")
    log(f"No refresh API found for {bp.get_name()}, continuing with compile only.")


def compile_blueprint(bp) -> None:
    try:
        unreal.KismetEditorUtilities.compile_blueprint(bp)
        log(f"Compiled {bp.get_name()}")
    except Exception as exc:
        log(f"Compile failed for {bp.get_name()}: {exc}")


def save_asset(asset) -> None:
    try:
        unreal.EditorAssetLibrary.save_loaded_asset(asset)
        log(f"Saved {asset.get_name()}")
    except Exception as exc:
        log(f"Save failed for {asset.get_name()}: {exc}")


for asset_path in ASSET_PATHS:
    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not asset:
        log(f"Failed to load asset: {asset_path}")
        continue

    log(f"Loaded {asset_path} as {asset.get_class().get_name()}")
    refresh_blueprint(asset)
    compile_blueprint(asset)
    save_asset(asset)

log("Done.")
