using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Science.Applied.Electronics;
using System;

namespace Ideam.Unity.Lighting
{
    public class LightCircuit : MonoBehaviour, IElectricalCircuit
    {
        [System.Serializable]
        public struct IntegratedLight
        {
            public Light light;
            public Color fullPowerColor, minPowerColor;

        }

        public IntegratedLight[] integratedLights;

        [SerializeField]
        float currentIn, currentUsed, currentOut, minCurrent, fullCurrent;

        public float CurrentInput { get { return currentIn; } set { ChangeAvailableCurrent(value); } }

        

        public float CurrentThroughPut { get { return currentOut; } }
        public float CurrentUsage { get { return currentUsed; } }
        public bool InputGate { get; set; }
        public bool OutputGate { get; set; }

        void Start()
        {

        }

        void Update()
        {

        }
        

        private void ChangeAvailableCurrent(float value)
        {
            currentIn = value;
            if(currentIn > minCurrent)
            {
                
                if(currentIn > fullCurrent)
                {
                    currentUsed = fullCurrent;
                    currentOut = currentIn - fullCurrent;
                    OutputGate = true;
                }
                else
                {
                    currentUsed = currentIn;
                    currentOut = 0f;
                    OutputGate = false;
                }

                float p = currentUsed / fullCurrent;
                for(int i =0; i < integratedLights.Length; i++)
                {
                    var il = integratedLights[i];
                    if (!InputGate)
                    {
                        il.light.enabled = true;
                    }
                    il.light.color = Color.LerpUnclamped(il.minPowerColor, il.fullPowerColor, p);
                }
            }
            else if (InputGate)
            {
                for(int i = 0; i < integratedLights.Length; i++)
                {
                    integratedLights[i].light.enabled = false;
                }
            }
        }

        public void ToggleLight()
        {
            if (InputGate)
            {
                CurrentInput = 0f;
            }
            else
            {
                CurrentInput = fullCurrent;
            }
        }

        public void SetCurrent(float newCurrent)
        {
            CurrentInput = newCurrent;
        }
    }
}