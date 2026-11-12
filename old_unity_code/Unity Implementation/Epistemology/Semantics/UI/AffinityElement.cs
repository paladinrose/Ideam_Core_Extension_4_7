using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UIElements;

using Ideam.Axiology.Valuation;
using Ideam.Science.Formal.Mathematics;

namespace Ideam.Unity.UI
{
    public class AffinityElement : SemanticsElement
    {
        // protected new IAffinity semanticObject;
        [SerializeReference]
        protected IAffinity affinityObject;
        public AffinityElement() : base() { }
        public AffinityElement(string label) : base(label) { }

        public AffinityElement(IAffinity affinityObject) : base(affinityObject)
        {
            this.affinityObject = affinityObject;
        }

        public AffinityElement(IAffinity affinityObject, string label): base(affinityObject, label) 
        {
           this.affinityObject = affinityObject;
        }

        
    }
}