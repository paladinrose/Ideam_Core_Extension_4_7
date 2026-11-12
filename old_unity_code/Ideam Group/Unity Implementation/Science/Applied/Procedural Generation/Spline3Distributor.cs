using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Pool;

using Ideam.Unity.Science.Formal.Mathematics;
using System;

namespace Ideam.Unity.ProceduralGeneration
{
    [System.Serializable]
    public class DistributionPool
    {
        public GameObject distributionObject;
        private ObjectPool<GameObject> pool;
        List<GameObject> activeObjects;
        public void SetupPool(int capacity)
        {
            activeObjects = new List<GameObject>();
            pool = new ObjectPool<GameObject>
                (
                () => { return GameObject.Instantiate(distributionObject); },
                GetObject, ReleaseObject, DestroyObject, true, capacity
                );
        }

        public void Clear()
        {
            for(int i =0; i < activeObjects.Count; i++)
            {
                pool.Release(activeObjects[i]);
            }
            activeObjects.Clear();
        }

        public GameObject GetObject()
        {
            return pool.Get();
        }

        private void DestroyObject(GameObject obj)
        {
            GameObject.Destroy(obj);
        }

        private void ReleaseObject(GameObject obj)
        {
            activeObjects.Remove(obj);
            obj.SetActive(false);
        }

        private void GetObject(GameObject obj)
        {
            activeObjects.Add(obj);
            obj.SetActive(true);
        }
    }

    public class Spline3Distributor : Spline3Behaviour
    {
        public DistributionPool[] distributionObjects;
        public DistributionSequencer[] distributionSequence;
        
        [System.Serializable]
        public struct DistributionSequencer
        {
            public int objectPoolID;

            //public Vector3 sequencePoint, sequenceDirection;

        }


        public enum DistributionSpacing { ByCount, ByDistance}
        public DistributionSpacing spacing;
        
        public enum DistributionMethod { Clamp, Mirror, Repeat}

        public DistributionMethod distributionMethod;

        public int distributionCount;

        public float distributionDistance;

        bool redistribute;

        private void Start()
        {
            for(int i=0; i < distributionObjects.Length; i++)
            {
                distributionObjects[i].SetupPool(10);
            }
        }

        private void OnEnable()
        {
            OnSplinePointUpdated += PointUpdated; 
        }
        
        private void OnDisable()
        {
            OnSplinePointUpdated -= PointUpdated;
        }

        private void PointUpdated(int pointID)
        {
            redistribute = true;
        }

        private void LateUpdate()
        {
            if (redistribute)
            {
                for(int i=0; i< distributionObjects.Length; i++)
                {
                    distributionObjects[i].Clear();
                }
                int numberOfObjectsToDistribute = 0;
               
                switch (spacing)
                {
                    case DistributionSpacing.ByCount:
                        numberOfObjectsToDistribute = distributionCount;
                        break;

                    case DistributionSpacing.ByDistance:
                        numberOfObjectsToDistribute = Mathf.FloorToInt(spline.TotalSplineLength / distributionDistance);
                        break;
                }
                float distributionScalar = 1f / numberOfObjectsToDistribute;

                switch (distributionMethod)
                {
                    case DistributionMethod.Clamp:
                        int numBeforeSequence = numberOfObjectsToDistribute - distributionSequence.Length;
                        
                        if(numBeforeSequence > 0)
                        {
                            numBeforeSequence /= 2;
                            for(int i =0; i < numBeforeSequence; i++)
                            {
                                float p = distributionScalar * i;
                                GameObject toDistribute = distributionObjects[distributionSequence[0].objectPoolID].GetObject();
                                Vector3 pos = spline.GetPoint(p).ToVector3();
                                //pos.Scale(distributionSequence[0].sequenceDirection);
                                toDistribute.transform.position = pos;
                            }
                            
                            int l = distributionSequence.Length;
                            for(int i = 0; i < l; i++)
                            {
                                float p = distributionScalar * (numBeforeSequence + i);
                                GameObject toDistribute = distributionObjects[distributionSequence[i].objectPoolID].GetObject();
                                Vector3 pos = spline.GetPoint(p).ToVector3();
                                //pos.Scale(distributionSequence[i].sequenceDirection);
                                toDistribute.transform.position = pos;
                            }

                            int afterStart = numBeforeSequence + l;
                            l--;
                            for(int i = afterStart; i < numberOfObjectsToDistribute; i++)
                            {
                                float p = distributionScalar * i;
                                GameObject toDistribute = distributionObjects[distributionSequence[l].objectPoolID].GetObject();
                                Vector3 pos = spline.GetPoint(p).ToVector3();
                                //pos.Scale(distributionSequence[l].sequenceDirection);
                                toDistribute.transform.position = pos;
                            }

                        }
                        else 
                        { 
                        }

                        break;

                    case DistributionMethod.Mirror:
                        int mirrorDistID = 0;
                        bool pingPong = false;
                        for(int i =0; i < numberOfObjectsToDistribute; i++)
                        {
                            float p = distributionScalar * i;
                            GameObject toDistribute = distributionObjects[distributionSequence[mirrorDistID].objectPoolID].GetObject();
                            Vector3 pos = spline.GetPoint(p).ToVector3();
                            //pos.Scale(distributionSequence[mirrorDistID].sequenceDirection);
                            toDistribute.transform.position = pos;

                            if (!pingPong)
                            {
                                mirrorDistID++;
                                if (mirrorDistID >= distributionSequence.Length)
                                {
                                    pingPong = true;
                                    mirrorDistID = distributionSequence.Length - 1;
                                }
                            }
                            else
                            {
                                mirrorDistID--;
                                if(mirrorDistID < 0) 
                                {
                                    pingPong = false;
                                    mirrorDistID = 0;
                                }
                            }
                        }
                        break;

                    case DistributionMethod.Repeat:
                        int distID = 0;

                        for (int i = 0; i < numberOfObjectsToDistribute; i++)
                        {
                            float p = distributionScalar * i;
                            GameObject toDistribute = distributionObjects[distributionSequence[distID].objectPoolID].GetObject();
                            Vector3 pos = spline.GetPoint(p).ToVector3();
                            //pos.Scale(distributionSequence[distID].sequenceDirection);
                            toDistribute.transform.position = pos;

                            if (distID >= distributionSequence.Length)
                            {
                                distID = 0;
                            }
                           
                        }
                        break;
                }

                redistribute = false;
            }
        }

        private GameObject GetObjectForDistribution(int id)
        {
            if(id < 0 || id >= distributionObjects.Length) { return null; }

            return distributionObjects[id].GetObject();
        }
    }
}