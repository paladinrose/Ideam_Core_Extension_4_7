using System.Linq;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

using Ideam.Unity.UI;
using Ideam.Editor;
using Ideam.Unity;

using UnityEngine.UIElements;
using System;
using Ideam.Unity.Semantics;
using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology
{
    [CustomEditor(typeof(UnityGameBoard))]
    public class UnityGameBoardEditor : EditorUserInterface<UnityGameBoard, UnityGameBoardUI>
    {
        protected override string[] ExcludedProperties()
        {
            return base.ExcludedProperties();//.Concat(new string[] { "musicZones", "game", "gamePieces", "languageAsset" }).ToArray();
        }


    }
}