using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Science.Applied.Layout;
namespace Ideam.Unity.Science.Applied.Layout
{
    public class GameObjectLayout : MonoBehaviour, ILayout
    {
        public void SetupLayout<T>(T[] layoutElements)
        {
            if (typeof(T) == typeof(GameObject))
            {
                GameObject[] layoutObjects = new GameObject[layoutElements.Length];
                for (int i = 0; i < layoutObjects.Length; i++)
                {
                    layoutObjects[i] = layoutElements[i] as GameObject;
                }

                SetupLayout(layoutObjects);
            }
        }

        public virtual void SetupLayout(GameObject[] layoutObjects)
        {

        }

        public virtual void PerformLayout()
        {

        }
    }

   
}