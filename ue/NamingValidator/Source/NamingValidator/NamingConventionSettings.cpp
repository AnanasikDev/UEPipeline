#include "NamingConventionSettings.h"

#define DEFAULT_RULE(CLASS_PATH, PREFIX, SUFFIX) \
    { \
        FNamingRuleEntry Entry; \
        Entry.AssetClass = TSoftClassPtr<UObject>(FSoftClassPath(TEXT(CLASS_PATH))); \
        Entry.Rules.Add({ TEXT(PREFIX), TEXT(SUFFIX) }); \
        Rules.Add(MoveTemp(Entry)); \
    }

UNamingConventionSettings::UNamingConventionSettings()
{
    // These defaults are only used the first time (before config is saved).
    // After that, the saved config in DefaultEditor.ini takes over.
    // Thanks to this, I don't need to enter all of these manually, but just expose them (and be able to modify in project settings)

    // Meshes
    DEFAULT_RULE("/Script/Engine.StaticMesh", "SM_", "");
    DEFAULT_RULE("/Script/Engine.SkeletalMesh", "SK_", "");

    // Textures / Materials
    DEFAULT_RULE("/Script/Engine.Texture", "T_", "");
    DEFAULT_RULE("/Script/Engine.Texture2D", "T_", "");
    DEFAULT_RULE("/Script/Engine.Material", "M_", "");
    DEFAULT_RULE("/Script/Engine.Material", "MM_", "");
    DEFAULT_RULE("/Script/Engine.MaterialInstance", "MI_", "");
    DEFAULT_RULE("/Script/Engine.MaterialFunction", "MF_", "");

    // Audio
    DEFAULT_RULE("/Script/Engine.SoundWave", "S_", "");
    DEFAULT_RULE("/Script/Engine.SoundCue", "SC_", "");

    // Animation
    DEFAULT_RULE("/Script/Engine.AnimSequence", "A_", "");
    DEFAULT_RULE("/Script/Engine.AnimMontage", "AM_", "");
    DEFAULT_RULE("/Script/Engine.BlendSpace", "BS_", "");
    DEFAULT_RULE("/Script/Engine.BlendSpace1D", "BS1D_", "");
    DEFAULT_RULE("/Script/Engine.AnimBlueprint", "ABP_", "");

    // Blueprints
    DEFAULT_RULE("/Script/Engine.Blueprint", "BP_", "");

    // FX
    DEFAULT_RULE("/Script/Engine.ParticleSystem", "PS_", "");
    DEFAULT_RULE("/Script/Niagara.NiagaraSystem", "NS_", "");
    DEFAULT_RULE("/Script/Niagara.NiagaraEmitter", "NE_", "");

    // Data
    DEFAULT_RULE("/Script/Engine.DataAsset", "DA_", "");
    DEFAULT_RULE("/Script/Engine.DataTable", "DT_", "");
    DEFAULT_RULE("/Script/Engine.CurveFloat", "CF_", "");
    DEFAULT_RULE("/Script/Engine.CurveVector", "CV_", "");
    DEFAULT_RULE("/Script/Engine.CurveLinearColor", "CLC_", "");

    // Input
    DEFAULT_RULE("/Script/EnhancedInput.InputAction", "IA_", "");
    DEFAULT_RULE("/Script/EnhancedInput.InputMappingContext", "IMC_", "");

    // AI
    DEFAULT_RULE("/Script/AIModule.BehaviorTree", "BT_", "");
    DEFAULT_RULE("/Script/AIModule.BlackboardData", "BB_", "");

    // UI
    DEFAULT_RULE("/Script/Engine.Font", "F_", "");

    // Levels / World
    DEFAULT_RULE("/Script/Engine.World", "L_", "");
}

#undef DEFAULT_RULE