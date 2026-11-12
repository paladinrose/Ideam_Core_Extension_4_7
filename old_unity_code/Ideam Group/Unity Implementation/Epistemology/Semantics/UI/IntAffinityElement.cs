using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UIElements;

using Ideam.Axiology.Valuation;
using Ideam.Science.Formal.Mathematics;

namespace Ideam.Unity.UI
{
    public class IntAffinityElement : AffinityElement
    {
        public IntAffinityElement() : base() { }

        public IntAffinityElement(string label) : base(label) { }

        public IntAffinityElement(IAffinity affinityObject) : base(affinityObject) { }
        
        public IntAffinityElement(IAffinity affinityObject, string label) : base(affinityObject, label) 
        { }

        public override void BuildList(string[] listEntries, int[] listIDs = null)
        {
            base.BuildList(listEntries, listIDs);
            Int[] affinities = affinityObject.GetSemanticAffinities<Int>();

            for (int i = 0; i < listElements.Length; i++)
            {
                int id = i;
                IntegerField affinityField = new IntegerField(affinities[i]);
                affinityField.isDelayed = true;
                affinityField.RegisterValueChangedCallback(evt =>
                {
                    affinityObject.SetSememeAffinity<Int>(id, affinityField.value);
                });
                listElements[i].content.Add(affinityField);
            }
        }

        protected override void AddSemantic(int id)
        {
            base.AddSemantic(id);
            IntegerField affinityField = new IntegerField(affinityObject.GetSememeAffinity<Int>(id));
            affinityField.isDelayed = true;
            affinityField.RegisterValueChangedCallback(evt =>
            {
                affinityObject.SetSememeAffinity<Int>(id, affinityField.value);
            });
            listElements[id].content.Add(affinityField);
        }
    }
}