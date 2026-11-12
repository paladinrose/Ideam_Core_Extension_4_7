using UnityEngine;
using UnityEngine.UIElements;

using Ideam.Unity.UI;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology
{
    [UserInterface(typeof(UnityGamePlace))]
    public class UnityGamePlaceUI : UnityGamePieceUI<UnityGamePlace> 
    {

        protected VisualElement boundsLine;
        protected BoundsField boundsField;
        public Toggle drawBounds;

        protected override bool SetupUIFromUXML()
        {
            if (!base.SetupUIFromUXML()) return false;

            gamePieceFoldout.text = "Game Place";

            boundsLine = new VisualElement();
            boundsLine.style.flexDirection = FlexDirection.Row;
            gamePieceFoldout.Add(boundsLine);

            drawBounds = new Toggle() { value = false };
            boundsLine.Add(drawBounds);
            boundsField = new BoundsField("Place Bounds") { value = sourceObject.placeBounds };
            boundsField.RegisterValueChangedCallback(ChangePlaceBounds);
            boundsLine.Add(boundsField);

            return true;
        }

        protected virtual void ChangePlaceBounds(ChangeEvent<Bounds> evt)
        {
            sourceObject.placeBounds = evt.newValue;

        }
    }
}