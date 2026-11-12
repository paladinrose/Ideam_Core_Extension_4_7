using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UIElements;

using Ideam.Unity.UI;
using Ideam.Science.Formal.Mathematics;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology
{
    public class UnityGamePieceResourcesDrop : SemanticsElement
    {
        #region UXML Factory
        
        public new class UxmlFactory : UxmlFactory<UnityGamePieceResourcesDrop, UxmlTraits> { }

        public new class UxmlTraits : SemanticsElement.UxmlTraits
        {

            public override IEnumerable<UxmlChildElementDescription> uxmlChildElementsDescription
            {
                get { yield break; }
            }

            public override void Init(VisualElement ve, IUxmlAttributes bag, CreationContext cc)
            {
                base.Init(ve, bag, cc);
                var propertiesDrop = ve as UnityGamePieceResourcesDrop;

            }
        }

        #endregion

        protected UnityGamePiece gamePiece;
        public UnityGamePiece GamePiece { get { return gamePiece; } set { if (gamePiece != value) { semanticObject = gamePiece = value; BuildSemanticsList(); } } }


        public virtual event Action<int, int> OnSetResourceValue, OnSetResourceMaxValue;

        public UnityGamePieceResourcesDrop() : base()
        {
            Label = "Resources";
        }

        public UnityGamePieceResourcesDrop(UnityGamePiece gamePiece) : base()
        {
            Label = "Resources";
           
            this.gamePiece = gamePiece;
            semanticObject = gamePiece;

            BuildSemanticsList();
        }

        public UnityGamePieceResourcesDrop(string label, UnityGamePiece gamePiece) : base(label)
        {
            this.gamePiece = gamePiece;
            semanticObject = gamePiece;
            BuildSemanticsList();
        }

        protected override void BuildSemanticsList()
        {
            languageAsset = gamePiece.languageAsset;

            if (!languageAsset)
            {
                languageAsset = UnityLanguageAsset.CurrentLanguageAsset;
            }

            if (!languageAsset)
            {
                Add(new Label("No Language Asset could be located."));
                return;
            }

            sememeIDs = gamePiece.GetResourceSememeIDs();
            string[] sememeLabels = languageAsset.GetFormatted(sememeIDs);

            BuildList(sememeLabels); //, sememeIDs);

        }

        public override void BuildList(string[] listEntries, int[] listIDs = null)
        {
            base.BuildList(listEntries, listIDs);
            Int[] values = gamePiece.GetResourceValues();
            Int[] maxValues = gamePiece.GetResourceMaxValues();

            for (int i = 0; i < listElements.Length; i++)
            {
                int id = i;
                IntegerField resourceField = new IntegerField() { value = values[id]};
                resourceField.isDelayed = true;
                resourceField.RegisterValueChangedCallback(evt =>
                {
                    var res = gamePiece.resources[id];
                    res.value = resourceField.value;
                    gamePiece.resources[id] = res;
                    OnSetResourceValue?.Invoke(id, resourceField.value);
                });
                
                listElements[i].content.Add(resourceField);

                IntegerField resourceMaxField = new IntegerField("/") { value = maxValues[id] };
                resourceMaxField.isDelayed = true;
                resourceMaxField.RegisterValueChangedCallback(evt =>
                {
                    var max = gamePiece.resources[id];
                    max.maxValue = resourceMaxField.value;
                    gamePiece.resources[id] = max;
                    OnSetResourceMaxValue?.Invoke(id, resourceMaxField.value);
                });

                listElements[i].content.Add(resourceMaxField);
            }
        }

        protected override void AddSemantic(int id)
        {
            string newSemanticLabel = listElements[id].Label;

            newSemanticLabel = languageAsset.ValidateGrammar(newSemanticLabel);

            if (newSemanticLabel == languageAsset.Invalid)
            {
                return;
            }

            int newSememeID = languageAsset.GetSemanticID(newSemanticLabel);

            //Debug.Log("Adding " + newSemanticLabel + " to language asset in slot " + newSememeID);

            gamePiece.AddResource(newSememeID);

            IntegerField resourceField = new IntegerField() { value = gamePiece.resources[id].value };
            resourceField.isDelayed = true;
            resourceField.RegisterValueChangedCallback(evt =>
            {
                var res = gamePiece.resources[id];
                res.value = resourceField.value;
                gamePiece.resources[id] = res;
            });
            listElements[id].content.Add(resourceField);

            IntegerField resourceMaxField = new IntegerField("/") { value = gamePiece.resources[id].maxValue };
            resourceMaxField.isDelayed = true;
            resourceMaxField.RegisterValueChangedCallback(evt =>
            {
                var max = gamePiece.resources[id];
                max.maxValue = resourceMaxField.value;
                gamePiece.resources[id] = max;
            });

            listElements[id].content.Add(resourceMaxField);
        }

        protected override void RemoveSemantic(int id)
        {
            gamePiece.RemoveResource(id);
        }
    }
}