using UnityEngine; 

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Animation
{
    public enum AnimatedPropertyType
    {
        Bool, Trigger, Float
    }
    [System.Serializable]
    public struct AnimatedProperty
    {
        public int animatorID;
        public int propertyID;
        public string propertyName;
        public float propertyValue;
        public AnimatedPropertyType propertyType;

        public bool BoolValue 
        {
            get { return (propertyValue != 0); } 
            set
            {
                if (value) { propertyValue = 1; } else { propertyValue = 0; }
            }
        }

        public float value { get { return propertyValue; } set { propertyValue = value; } }
    }
}