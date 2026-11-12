using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.Gameplay.CharacterControls
{
    public class ClingControl : BaseControl
    {
        public event Action<int, GameObject> OnStartClinging, OnStopClinging;
        public event Action<int> OnClingUpdate;
        public enum ClingMethod
        {
            Parent,
            LockClingerToClingTarget,
            LockClingTargetToClinger,
            DirectSet,
            EqualForces,
            ForceToClinger,
            ForceToClingTarget
        }

        [System.Serializable]
        public struct ClingData
        {
            public bool isClinging;
            public float clingStrength;
            public ClingMethod clingMethod;
            public Transform clinger, clingTarget, clingTargetParent;
            public Rigidbody clingerBody, clingTargetBody;

            public TriggerSensor clingSensor;
            public JointSensor jointBreakSensor;
            
            public FixedJoint clingJoint;

            public Vector3 clingAtPoint, clingAtAngle, currentPositionDifference, currentAngleDifference;

            public float clingBreakForce,
                         clingBreakTorque;
            public bool clingTargetUsesGravity;

            public ClingData(ClingData original)
            {
                isClinging = original.isClinging;
                clingStrength = original.clingStrength;
                clingMethod = original.clingMethod;
                clinger = original.clinger;
                clingTarget = original.clingTarget;
                clingTargetParent = original.clingTargetParent;
                clingerBody = original.clingerBody;
                clingTargetBody = original.clingTargetBody;
                clingSensor = original.clingSensor;
                jointBreakSensor = original.jointBreakSensor;

                clingAtAngle = original.clingAtAngle;
                clingAtPoint = original.clingAtPoint;
                currentAngleDifference = original.currentAngleDifference;
                currentPositionDifference = original.currentPositionDifference;

                clingBreakForce = original.clingBreakForce;
                clingBreakTorque = original.clingBreakTorque;
                clingTargetUsesGravity = original.clingTargetUsesGravity;
                clingJoint = original.clingJoint;
            }
        }


        public ClingData[] clingData = new ClingData[0];

        public int StartCling(ClingData cling)
        {
            //if (cling.clingTarget == null) { return; }

            //Debug.Log("Trying to cling to " + cling.clingTarget.name);

            int clingingID = -1, firstEmpty = -1;
            for (int i = 0; i < clingData.Length; i++)
            {
                if (clingData[i].isClinging)
                {
                    if (clingData[i].clinger == cling.clinger && clingData[i].clingTarget == cling.clingTarget)
                    {
                        clingingID = i;
                        break;
                    }
                    
                } else if( firstEmpty < 0 && clingData[i].clinger == cling.clinger) { firstEmpty = i; }
            }

            if (clingingID >= 0)
            {
                //We're already clinging to this object with this clinger. 
                //We want to update the clinging values then finish.
                ClingData updatedCling = clingData[clingingID];
                
                updatedCling.clingBreakForce = cling.clingBreakForce;
                updatedCling.clingBreakTorque = cling.clingBreakTorque;

                clingData[clingingID] = updatedCling;
                return clingingID;
            }
            else if (firstEmpty >= 0)
            {
                ClingData data = new ClingData(cling);

                if (data.clingTarget)
                {
                    data.clingTargetBody = data.clingTarget.GetComponent<Rigidbody>();

                    if (data.clingerBody)
                    {
                        data.currentPositionDifference = data.clingTarget.position - data.clinger.position;
                        data.currentAngleDifference = Quaternion.FromToRotation(data.clingTarget.eulerAngles, data.clinger.eulerAngles).eulerAngles;

                        if (data.clingTargetBody)
                        {
                            if (data.clingTargetBody.isKinematic)
                            {
                                data.clingMethod = ClingMethod.DirectSet;
                            }
                            else
                            {
                                data.clingMethod = ClingMethod.EqualForces;

                                data.clingTargetUsesGravity = data.clingTargetBody.useGravity;
                                //data.clingTargetBody.useGravity = false;

                                data.clingJoint = data.clingerBody.gameObject.AddComponent<FixedJoint>();
                                data.clingJoint.breakForce = data.clingBreakForce;
                                data.clingJoint.breakTorque = data.clingBreakTorque;
                                data.clingJoint.connectedBody = data.clingTargetBody;
                                data.jointBreakSensor.SetJoint(data.clingJoint);

                            }
                        }
                        else
                        {
                            if (data.clingTarget.gameObject.isStatic)
                            {
                                data.clingMethod = ClingMethod.ForceToClinger;

                            }
                            else
                            {
                                data.clingMethod = ClingMethod.Parent;
                                data.clingTargetParent = data.clingTarget.parent;
                                data.clingTarget.SetParent(data.clinger, true);
                            }
                        }
                    }
                    else
                    {
                        data.currentPositionDifference = data.clingTarget.position - data.clinger.position;
                        data.currentAngleDifference = Quaternion.FromToRotation(data.clingTarget.eulerAngles, data.clinger.eulerAngles).eulerAngles;

                        if (data.clingTargetBody)
                        {
                            if (data.clingTargetBody.isKinematic)
                            {
                                data.clingMethod = ClingMethod.DirectSet;
                            }
                            else
                            {
                                data.clingMethod = ClingMethod.ForceToClingTarget;
                            }
                        }
                        else
                        {
                            if (data.clingTarget.gameObject.isStatic)
                            {
                                data.clingMethod = ClingMethod.LockClingerToClingTarget;

                            }
                            else
                            {
                                data.clingMethod = ClingMethod.Parent;
                                data.clingTargetParent = data.clingTarget.parent;
                                data.clingTarget.SetParent(data.clinger, true);

                            }
                        }
                    }
                }

                clingData[firstEmpty] = data;
                GameObject clingObj = null;
                if (data.clingTarget) { clingObj = data.clingTarget.gameObject; }
                OnStartClinging?.Invoke(firstEmpty, clingObj);
                return firstEmpty;
            }

            return -1;
        }
        
        public void UpdateClingValues(int id, float clingStrength)
        {
            ClingData cling = clingData[id];
            cling.clingStrength = clingStrength;
            clingData[id] = cling;
        }
        
        public void StopClinging(int id)
        {
            if (id >= clingData.Length || id < 0) return;

            ClingData cling = clingData[id];
            if(cling.clingTarget) 
            {
                GameObject wasClinging = cling.clingTarget.gameObject;
                cling.clingStrength = 0;
                cling.isClinging = false;
                if(cling.clingMethod == ClingMethod.Parent) { wasClinging.transform.SetParent(cling.clingTargetParent, true); }
                cling.clingTarget = null;
                cling.clingTargetBody = null;
                cling.clingTargetParent = null;
                clingData[id] = cling;
                OnStopClinging?.Invoke(id, wasClinging);
            }
            clingData[id] = cling;
        }

        public void ClingUpdate()
        {
            for(int i =0; i < clingData.Length; i++)
            {
                ClingData cling = clingData[i];

                if(!cling.clinger || !cling.clingTarget) { return; }
                bool didUpdate = false;
                switch (cling.clingMethod)
                {
                    case ClingMethod.Parent:
                        break;
                    case ClingMethod.LockClingerToClingTarget:
                        break;
                    case ClingMethod.LockClingTargetToClinger:
                        break;

                    case ClingMethod.DirectSet:
                        DirectSetCling(cling); 
                        didUpdate = true;
                        break;

                    case ClingMethod.EqualForces:
                        break;

                    case ClingMethod.ForceToClinger:
                        cling.clingerBody.MovePosition(cling.clingTarget.position + cling.currentPositionDifference);
                        didUpdate = true;
                        break;

                    case ClingMethod.ForceToClingTarget:
                        Vector3 heldDiff = (cling.clinger.position + cling.currentPositionDifference) - cling.clingTargetBody.position;
                        cling.clingTargetBody.AddForce(heldDiff, ForceMode.VelocityChange);
                        didUpdate = true;
                        break;
                }

                if (didUpdate) { OnClingUpdate?.Invoke(i); }
            }
        }

        private void DirectSetCling(ClingData cling)
        {
            if (controlFlags.HasFlag(ControlFlags.Physical))
            {
                cling.clingTargetBody.MovePosition(cling.clingerBody.position + cling.currentPositionDifference);
                cling.clingTargetBody.MoveRotation(cling.clingerBody.rotation * Quaternion.Euler(cling.currentAngleDifference));
            }
            else
            {
                cling.clingTargetBody.MovePosition(cling.clinger.position + cling.currentPositionDifference);
                cling.clingTargetBody.MoveRotation(cling.clinger.rotation * Quaternion.Euler(cling.currentAngleDifference));
            }
        }

    }
}
