using System;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;
using UnityEngine.UIElements;

using Ideam.Unity.UI;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology
{
    [UserInterface(typeof(UnityGameBoard))]
    public class UnityGameBoardUI : UserInterface<UnityGameBoard>
    {
        public override void SubscribeToObjectEvents()
        {

        }

        public override void UnsubscribeFromObjectEvents()
        {

        }

        protected override bool SetupUIFromUXML()
        {
            if (!base.SetupUIFromUXML()) { return false; }


            return true;
        }
    }
}
