using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Unity.Axiology.Aesthetics.The_Arts.Animation;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.Gameplay.CharacterControls
{
    public class BaseControl : MonoBehaviour
    {
        [SerializeField]
        protected ControlFlags controlFlags;
        
        public virtual ControlFlags ControlFlags
        {
            get { return controlFlags; }
            set
            {
                controlFlags = value;
            }
        }
        
        public BaseController controller;
        public Transform controlTarget;
        public Rigidbody controlBody;

        public Animator[] controlAnimators;
        public AnimatedProperty[] animatedProperties;
        

        
        protected virtual void GetAnimatedPropertyIDs()
        {
            for(int i =0; i < animatedProperties.Length; i++)
            {
                AnimatedProperty ap = animatedProperties[i];
                ap.propertyID = Animator.StringToHash(ap.propertyName);
                animatedProperties[i] = ap;
            }
        }

        protected virtual void UpdateAnimatedProperties()
        {
            for(int i =0; i < animatedProperties.Length; i++)
            {
                AnimatedProperty ap = animatedProperties[i];
                if(ap.propertyID == 0) { continue; }

                switch (ap.propertyType)
                {
                    case AnimatedPropertyType.Bool:
                        controlAnimators[ap.animatorID].SetBool(ap.propertyID, ap.BoolValue);
                        break;
                    case AnimatedPropertyType.Trigger:
                        if (ap.BoolValue) { controlAnimators[ap.animatorID].SetTrigger(ap.propertyID); ap.BoolValue = false; }
                        break;
                    case AnimatedPropertyType.Float:
                        controlAnimators[ap.animatorID].SetFloat(ap.propertyID, ap.value);
                        break;
                }
                animatedProperties[i] = ap;
            }
        }

    }
}