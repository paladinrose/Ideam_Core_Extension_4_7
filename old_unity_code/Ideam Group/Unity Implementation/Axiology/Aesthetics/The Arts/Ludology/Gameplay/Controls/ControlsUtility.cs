using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.Gameplay.CharacterControls
{
    [Flags]
    public enum ControlFlags
    {
        Disabled = 0,
        Animated = 1,
        RootMotion = 2,
        Physical = 4,
        Kinematic = 8, 
        Direct = 16
    }

    public static class ControlsUtility
    {

    }
}