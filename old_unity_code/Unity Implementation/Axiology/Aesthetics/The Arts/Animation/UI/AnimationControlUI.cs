using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Unity.UI;
namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Animation
{
    public class AnimationControlUI : UserInterface<AnimationControl>
    {
        public override void SubscribeToObjectEvents()
        {

        }

        public override void UnsubscribeFromObjectEvents()
        {

        }

        protected override bool SetupUIFromUXML()
        {
            if (!base.SetupUIFromUXML()) return false;


            return true;
        }
    }
}