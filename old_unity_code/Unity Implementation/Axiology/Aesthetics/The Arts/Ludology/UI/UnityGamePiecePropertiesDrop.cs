using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UIElements;

using Ideam.Unity.UI;
using Ideam.Science.Formal.Mathematics;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology
{
    public class UnityGamePiecePropertiesDrop : SemanticsElement
    {
        #region UXML Factory

        public new class UxmlFactory : UxmlFactory<UnityGamePiecePropertiesDrop, UxmlTraits> { }

        public new class UxmlTraits : SemanticsElement.UxmlTraits
        {

            public override IEnumerable<UxmlChildElementDescription> uxmlChildElementsDescription
            {
                get { yield break; }
            }

            public override void Init(VisualElement ve, IUxmlAttributes bag, CreationContext cc)
            {
                base.Init(ve, bag, cc);
                var propertiesDrop = ve as UnityGamePiecePropertiesDrop;

            }
        }

        #endregion



        protected UnityGamePiece gamePiece;
        public UnityGamePiece GamePiece { get { return gamePiece; } set { if(gamePiece != value) {semanticObject = gamePiece = value; BuildSemanticsList(); } } }
        public UnityGamePiecePropertiesDrop() : base()
        {
            Label = "Properties";
        }

        public virtual event Action<int, int> OnSetPropertyValue;
        public UnityGamePiecePropertiesDrop(UnityGamePiece gamePiece) : base()
        {
            Label = "Properties";
           
            this.gamePiece = gamePiece;
            semanticObject = gamePiece;
            BuildSemanticsList();
        }

        public UnityGamePiecePropertiesDrop(string label, UnityGamePiece gamePiece) : base(label)
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

            sememeIDs = gamePiece.GetSememeIDs();
            string[] sememeLabels = languageAsset.GetFormatted(sememeIDs);

            BuildList(sememeLabels); //, sememeIDs);

        }

        public override void BuildList(string[] listEntries, int[] listIDs = null)
        {
            base.BuildList(listEntries, listIDs);
            Int[] affinities = gamePiece.GetSemanticAffinities();

            for (int i = 0; i < listElements.Length; i++)
            {
                int id = i;
                IntegerField affinityField = new IntegerField() { value = affinities[id] };
                affinityField.isDelayed = true;
                affinityField.RegisterValueChangedCallback(evt =>
                {
                    var prop = gamePiece.properties[id];
                    prop.value = affinityField.value;
                    gamePiece.properties[id] = prop;
                });

                listElements[i].content.Add(affinityField);
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

            gamePiece.AddSememe(newSememeID);

            IntegerField affinityField = new IntegerField() { value = gamePiece.GetSememeAffinity<Int>(id) };
            affinityField.isDelayed = true;
            affinityField.RegisterValueChangedCallback(evt =>
            {
                gamePiece.SetSememeAffinity<Int>(id, affinityField.value);
                OnSetPropertyValue?.Invoke(id, affinityField.value);
            });
            listElements[id].content.Add(affinityField);
        }

        protected override void RemoveSemantic(int id)
        {
            gamePiece.RemoveSememe(id);
        }
    }
}