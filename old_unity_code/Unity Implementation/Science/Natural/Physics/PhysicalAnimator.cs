using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;
using Ideam.Unity.Axiology.Aesthetics.The_Arts.Animation;

namespace Ideam.Unity.Science.Natural.Physics
{
    public class PhysicalAnimator : MonoBehaviour
    {
        public static SynchronizerSettings DefaultSyncSettings = new SynchronizerSettings() 
        {
              positionDamper = 2, positionMinForce = 500, positionMaxForce = 2000, positionSpring = 1000,
              rotationDamper = 2, rotationMinForce = 500, rotationMaxForce = 2000, rotationSpring = 1000
        };


        public AnimationBehaviour animationSource;
        public Transform[] physicsRoots = new Transform[0], synchronizerRoots = new Transform[0];
        public PhysicsBone[] physicsBones = new PhysicsBone[0];
        public PhysicsSkinner[] physicsSkinners = new PhysicsSkinner[0];
        
        #region Physics Properties

        [SerializeField]
        protected int solverIterations = 12;
        public int SolverIterations
        {
            get { return solverIterations; }
            set
            {
                if (value != solverIterations)
                {
                    solverIterations = value;
                    for (int i = 0; i < physicsBones.Length; i++)
                    {
                        physicsBones[i].body.solverIterations = solverIterations;
                        physicsBones[i].synchronizerBody.solverIterations = solverIterations;
                    }
                }
            }
        }


        [SerializeField]
        protected int solverVelocityIterations = 4;
        public int SolverVelocityIterations
        {
            get { return solverVelocityIterations; }
            set
            {
                if (value != solverVelocityIterations)
                {
                    solverVelocityIterations = value;
                    for (int i = 0; i < physicsBones.Length; i++)
                    {
                        physicsBones[i].body.solverVelocityIterations = solverVelocityIterations;
                        physicsBones[i].synchronizerBody.solverVelocityIterations = solverVelocityIterations;

                    }
                }
            }
        }


        [SerializeField]
        protected float maxAngularVelocity = 20;
        public float MaximumAngularVelocity
        {
            get { return maxAngularVelocity; }
            set
            {
                if (value != maxAngularVelocity)
                {
                    maxAngularVelocity = value;
                    for (int i = 0; i < physicsBones.Length; i++)
                    {
                        physicsBones[i].body.maxAngularVelocity = maxAngularVelocity;
                        physicsBones[i].body.maxAngularVelocity = maxAngularVelocity;
                    }
                }
            }
        }


        [SerializeField]
        float totalMass = -1;
        public float TotalMass
        {
            get
            {
                if (totalMass < 0)
                {
                    totalMass = 0;
                    for (int i = 0; i < physicsBones.Length; i++)
                    {
                        totalMass += physicsBones[i].body.mass;
                    }
                }
                return totalMass;
            }
            set
            {
                if (value > 0)
                {

                    bool reDisable = false;
                    if (!physicsEnabled) { reDisable = true; EnablePhysics(); }

                    float totalVolume = 0;
                    float massScalar = 1f / totalMass;

                    averageDensity = 0;

                    for (int i = 0; i < physicsBones.Length; i++)
                    {
                        totalVolume += physicsBones[i].collider.bounds.Volume();
                        physicsBones[i].body.mass = physicsBones[i].body.mass * massScalar * value;
                        physicsBones[i].density = physicsBones[i].body.mass / physicsBones[i].collider.bounds.Volume();
                        averageDensity += physicsBones[i].density;
                    }

                    averageDensity /= physicsBones.Length;

                    totalMass = value;
                    if (reDisable) { DisablePhysics(); }
                }
            }
        }

        

        [SerializeField]
        float averageDensity = 985f;
        public float AverageDensity
        {
            get
            {
                if (averageDensity < 0)
                {
                    averageDensity = 0;
                    for (int i = 0; i < physicsBones.Length; i++)
                    {
                        physicsBones[i].density = physicsBones[i].body.mass / physicsBones[i].collider.bounds.Volume();
                        averageDensity += physicsBones[i].density;
                    }
                    averageDensity /= physicsBones.Length;
                }
                return averageDensity;
            }
            set
            {
                if (value != averageDensity && value > 0)
                {
                    float densityScalar = averageDensity / value;
                    averageDensity = value;
                    totalMass = 0;
                    bool reDisable = false;
                    if (!physicsEnabled) { reDisable = true; EnablePhysics(); }
                    for (int i = 0; i < physicsBones.Length; i++)
                    {
                        physicsBones[i].density *= densityScalar;
                        physicsBones[i].body.mass = physicsBones[i].Volume() * physicsBones[i].density;
                        totalMass += physicsBones[i].body.mass;
                    }
                    if (reDisable) { DisablePhysics(); }
                }
            }
        }

        #endregion

        #region Enabling and Disabling Physics-based Animation

        [SerializeField]
        private bool physicsEnabled;

        public bool PhysicsEnabled
        {
            get { return physicsEnabled; }
            set
            {
                if (value == physicsEnabled)
                {
                    return;
                }

                physicsEnabled = value;

                if (!value) { DisablePhysics(); }
                else { EnablePhysics(); }

            }
        }

        private void EnablePhysics()
        {
            for (int i = 0; i < physicsRoots.Length; i++)
            {
                physicsRoots[i].gameObject.SetActive(true);
            }

            for (int i = 0; i < physicsBones.Length; i++)
            {
                TeleportPhysicsToAnimation(i);
            }

            for (int i = 0; i < physicsSkinners.Length; i++)
            {
                physicsSkinners[i].skinRenderer.bones = physicsSkinners[i].physicsBones;
                physicsSkinners[i].skinRenderer.rootBone = physicsSkinners[i].physicsRoot;
            }

        }

        private void DisablePhysics()
        {
            for (int i = 0; i < physicsSkinners.Length; i++)
            {
                physicsSkinners[i].skinRenderer.bones = physicsSkinners[i].animationBones;
                physicsSkinners[i].skinRenderer.rootBone = physicsSkinners[i].animationRoot;
            }

            for (int i = 0; i < physicsRoots.Length; i++)
            {
                physicsRoots[i].gameObject.SetActive(false);
            }
        }
        
        public void TeleportPhysicsToAnimation(int id)
        {
            Transform[] animationBones = animationSource.animationBones;
            physicsBones[id].body.position = animationBones[id].position;
            physicsBones[id].body.rotation = animationBones[id].rotation;
        }


        #endregion

        private void Awake()
        {
            for (int i = 0; i < physicsBones.Length; i++)
            {
               
                if (physicsBones[i].joint)
                    physicsBones[i].InitialRotation = physicsBones[i].transform.localRotation;
            }
        }

        private void FixedUpdate()
        {
            if (!physicsEnabled) { return; }

            for (int i = 0; i <physicsBones.Length; i++)
            {
                
                switch (physicsBones[i].AnimationMode)
                {
                    case PhysicsAnimationMode.NotAnimated:
                        break;
                    case PhysicsAnimationMode.MotorDriven:
                        physicsBones[i].joint.SetTargetRotationLocal(animationSource.animationBones[i].localRotation, physicsBones[i].InitialRotation);
                        break;
                    case PhysicsAnimationMode.Synchronized:
                        physicsBones[i].synchronizerBody.MoveRotation(animationSource.animationBones[i].rotation);
                        physicsBones[i].synchronizerBody.MovePosition(animationSource.animationBones[i].position);
                        
                        
                        break;
                }
                
            }
            
        }

        public static T GeneratePhysicalAnimator<T>(AnimationBehaviour source) where T : PhysicalAnimator
        {
            T physicalAnimator = source.gameObject.GetComponent<T>();

            if (physicalAnimator) return physicalAnimator;
            

            physicalAnimator = source.gameObject.AddComponent<T>();
            physicalAnimator.animationSource = source;

            physicalAnimator.CreatePhysicsSkeleton();

            return physicalAnimator;
        }

        protected virtual void CreatePhysicsSkeleton()
        {
            Transform[]animationRoots = animationSource.animationRoots, animationBones = animationSource.animationBones;
            List<Transform> newPhysBones = new List<Transform>(), newSyncs = new List<Transform>();

            physicsRoots = new Transform[animationRoots.Length];
            synchronizerRoots = new Transform[animationRoots.Length];

            for (int i = 0; i < animationRoots.Length; i++)
            {
                physicsRoots[i] = Instantiate(animationRoots[i]);

                physicsRoots[i].SetParent(animationRoots[i].parent);
                physicsRoots[i].localScale = animationRoots[i].localScale;
                physicsRoots[i].rotation = animationRoots[i].rotation;
                physicsRoots[i].position = animationRoots[i].position;

                newPhysBones.AddRange(physicsRoots[i].GetComponentsInChildren<Transform>());


                synchronizerRoots[i] = new GameObject(animationRoots[i].name + " (Sync Root)").transform;
                Transform sR = Instantiate(animationRoots[i]);

                synchronizerRoots[i].SetParent(animationRoots[i].parent);
                synchronizerRoots[i].localScale = animationRoots[i].localScale;
                synchronizerRoots[i].rotation = animationRoots[i].rotation;
                synchronizerRoots[i].position = animationRoots[i].position;
                sR.SetParent(synchronizerRoots[i]);
                sR.localScale = animationRoots[i].localScale;
                sR.rotation = animationRoots[i].rotation;
                sR.position = animationRoots[i].position;
                Transform[] syncKids = sR.GetComponentsInChildren<Transform>();
                newSyncs.AddRange(syncKids);
                for(int sk = 0; sk < syncKids.Length; sk++)
                {
                    syncKids[sk].SetParent(synchronizerRoots[i], true);
                }
            }

            physicsBones = new PhysicsBone[newPhysBones.Count];

            var newBones = new List<PhysicsBone>();
            var skins = new List<PhysicsSkinner>();
            for (int i = 0; i < animationBones.Length; i++)
            {
                newPhysBones[i].name = animationBones[i].name + " (Physics)";
                newSyncs[i].name = animationBones[i].name + " (Sync)";

                PhysicsBone newBone = new PhysicsBone();
                newBone.synchronizerSettings = DefaultSyncSettings;

                newBone.transform = newPhysBones[i];
                newBone.synchronizer = newSyncs[i];

                Rigidbody body = newBone.transform.gameObject.AddComponent<Rigidbody>();
                body.drag = 0f;
                body.angularDrag = 0f;
                body.useGravity = false;
                body.isKinematic = false;
                body.interpolation = RigidbodyInterpolation.Interpolate;
                body.solverIterations = solverIterations;
                body.solverVelocityIterations = solverVelocityIterations;
                body.maxAngularVelocity = maxAngularVelocity;
                newBone.body = body;

                newBone.density = averageDensity;
                
               
                GameObject syncObject = newSyncs[i].gameObject;
                Rigidbody syncBody = syncObject.AddComponent<Rigidbody>();
                syncBody.interpolation = RigidbodyInterpolation.Interpolate;
                syncBody.isKinematic = true;
                syncBody.useGravity = false;
                syncBody.mass = 0.001f;
                syncBody.drag = 0f;
                syncBody.angularDrag = 0f;
                syncBody.constraints = RigidbodyConstraints.FreezeAll;
                newBone.synchronizerBody = syncBody;


                ConfigurableJoint joint = syncObject.AddComponent<ConfigurableJoint>();
                joint.enablePreprocessing = false;

                SynchronizerSettings syncSettings = new SynchronizerSettings()
                {
                    positionDamper = DefaultSyncSettings.positionDamper * 0.5f,
                    positionMaxForce = DefaultSyncSettings.positionMaxForce * 5,
                    positionSpring = DefaultSyncSettings.positionSpring * 5,
                    rotationDamper = DefaultSyncSettings.rotationDamper * 0.5f,
                    rotationMaxForce = DefaultSyncSettings.rotationMaxForce * 5,
                    rotationSpring = DefaultSyncSettings.rotationSpring * 5
                };

                joint.xMotion = ConfigurableJointMotion.Free;
                joint.yMotion = ConfigurableJointMotion.Free;
                joint.zMotion = ConfigurableJointMotion.Free;
                joint.angularXMotion = ConfigurableJointMotion.Free;
                joint.angularYMotion = ConfigurableJointMotion.Free;
                joint.angularZMotion = ConfigurableJointMotion.Free;

                joint.rotationDriveMode = RotationDriveMode.Slerp;
                var rotationDrive = joint.slerpDrive;
                rotationDrive.maximumForce = syncSettings.rotationMaxForce;
                rotationDrive.positionSpring = syncSettings.rotationSpring;
                rotationDrive.positionDamper = syncSettings.rotationDamper;
                joint.slerpDrive = rotationDrive;

                var posDrive = joint.xDrive;
                posDrive.maximumForce = syncSettings.positionMaxForce;
                posDrive.positionSpring = syncSettings.positionSpring;
                posDrive.positionDamper = syncSettings.positionDamper;
                joint.xDrive = posDrive;

                posDrive = joint.yDrive;
                posDrive.maximumForce = syncSettings.positionMaxForce;
                posDrive.positionSpring = syncSettings.positionSpring;
                posDrive.positionDamper = syncSettings.positionDamper;
                joint.yDrive = posDrive;

                posDrive = joint.zDrive;
                posDrive.maximumForce = syncSettings.positionMaxForce;
                posDrive.positionSpring = syncSettings.positionSpring;
                posDrive.positionDamper = syncSettings.positionDamper;
                joint.zDrive = posDrive;

                joint.breakForce = float.PositiveInfinity;
                joint.breakTorque = float.PositiveInfinity;

                joint.projectionMode = JointProjectionMode.PositionAndRotation;
                joint.projectionDistance = 0.01f;
                joint.projectionAngle = 0.1f;

                joint.connectedBody = newBone.body;
                joint.anchor = Vector3.zero;
                newBone.synchronizerJoint = joint;

                newBone.synchronizer.gameObject.SetActive(false);
                newBone.AnimationMode = PhysicsAnimationMode.MotorDriven;

                newBones.Add(newBone);
            }

            physicsBones = newBones.ToArray();
            //With our parts setup and ready for colliders, etc, we add our parts list back to ragdoll,
            //and set up the joints and colliders.

            for (int i = 0; i < physicsBones.Length; i++)
            {
                ChangeBoneCollider(i, "Capsule");
                if (i > 0) { ChangeBoneJoint(i, "Hinge"); }
                else { physicsBones[i].AnimationMode = PhysicsAnimationMode.Synchronized; }
                physicsBones[i].body.mass = physicsBones[i].Volume() * physicsBones[i].density;
            }
            totalMass = -1;
            var tm = TotalMass;

            SkinnedMeshRenderer[] skinRenderers = animationSource.riggedAnimator.GetComponentsInChildren<SkinnedMeshRenderer>();

            for (int i = 0; i < skinRenderers.Length; i++)
            {
                PhysicsSkinner newSkin = new PhysicsSkinner();
                newSkin.animationBones = skinRenderers[i].bones;
                newSkin.animationRoot = skinRenderers[i].rootBone;
                newSkin.skinRenderer = skinRenderers[i];
                int bl = newSkin.animationBones.Length;
                newSkin.physicsBones = new Transform[bl];
                for (int p = 0; p < physicsBones.Length; p++)
                {
                    if (animationBones[p] == newSkin.animationRoot)
                    {
                        newSkin.physicsRoot = physicsBones[p].transform;
                    }
                    for (int b = 0; b < bl; b++)
                    {
                        if (animationBones[p] == newSkin.animationBones[b])
                        {
                            newSkin.physicsBones[b] =physicsBones[p].transform;
                        }
                    }
                }
                skins.Add(newSkin);
            }

            physicsSkinners = skins.ToArray();

            PhysicsEnabled = true;

            return;
        }

        [ContextMenu("Realign Syncs and Physics")]
        void ReAlignAllSyncsAndPhysicsBones()
        {
            for(int i =0; i < physicsBones.Length; i++)
            {
                //physicsBones[i].transform.localScale = physicsBones[i].synchronizer.localScale = animationSource.animationBones[i].localScale;
                physicsBones[i].transform.rotation = physicsBones[i].synchronizer.rotation = animationSource.animationBones[i].rotation;
                physicsBones[i].transform.position = physicsBones[i].synchronizer.position = animationSource.animationBones[i].position;

            }
        }


        protected void CreateSynchronizers()
        {
            Transform[] animationBones = animationSource.animationBones;
            int l = animationBones.Length;

            DestroySynchronizers(); 


            for (int i = 0; i < l; i++)
            {
                GameObject rootSync = new GameObject(animationBones[i].name + " (Sync)");
                rootSync.transform.SetParent(animationBones[i]);
                rootSync.transform.localEulerAngles = rootSync.transform.localPosition = Vector3.zero;

                Rigidbody body = rootSync.AddComponent<Rigidbody>();

                body.isKinematic = true;
                body.useGravity = false;
                //body.detectCollisions = false;
                body.drag = 0f;
                body.angularDrag = 0f;
                physicsBones[i].synchronizerBody = body;
                

                ConfigurableJoint joint = rootSync.AddComponent<ConfigurableJoint>();
                joint.enablePreprocessing = false;
                SynchronizerSettings syncSettings = physicsBones[i].synchronizerSettings;

                joint.xMotion = ConfigurableJointMotion.Free;
                joint.yMotion = ConfigurableJointMotion.Free;
                joint.zMotion = ConfigurableJointMotion.Free;
                joint.angularXMotion = ConfigurableJointMotion.Free;
                joint.angularYMotion = ConfigurableJointMotion.Free;
                joint.angularZMotion = ConfigurableJointMotion.Free;

                joint.rotationDriveMode = RotationDriveMode.Slerp;
                var rotationDrive = joint.slerpDrive;
                rotationDrive.maximumForce = syncSettings.rotationMaxForce;
                rotationDrive.positionSpring = syncSettings.rotationSpring;
                rotationDrive.positionDamper = syncSettings.rotationDamper;
                joint.slerpDrive = rotationDrive;

                var posDrive = joint.xDrive;
                posDrive.maximumForce = syncSettings.positionMaxForce;
                posDrive.positionSpring = syncSettings.positionSpring;
                posDrive.positionDamper = syncSettings.positionDamper;
                joint.xDrive = posDrive;

                posDrive = joint.yDrive;
                posDrive.maximumForce = syncSettings.positionMaxForce;
                posDrive.positionSpring = syncSettings.positionSpring;
                posDrive.positionDamper = syncSettings.positionDamper;
                joint.yDrive = posDrive;

                posDrive = joint.zDrive;
                posDrive.maximumForce = syncSettings.positionMaxForce;
                posDrive.positionSpring = syncSettings.positionSpring;
                posDrive.positionDamper = syncSettings.positionDamper;
                joint.zDrive = posDrive;

                joint.breakForce = float.PositiveInfinity;
                joint.breakTorque = float.PositiveInfinity;

                joint.projectionMode = JointProjectionMode.PositionAndRotation;
                joint.projectionDistance = 0.02f;
                joint.projectionAngle = .15f;

                joint.connectedBody = physicsBones[i].body;
                //joint.swapBodies = true;
                joint.anchor = Vector3.zero;
                physicsBones[i].synchronizer = joint.transform;
                physicsBones[i].synchronizerJoint = joint;
            }
        }

        [ContextMenu("Recalculate Total Mass")]
        public void ResetTotalMass()
        {
            totalMass = 0;
            for (int i = 0; i < physicsBones.Length; i++)
            {
                float m = physicsBones[i].Volume() * physicsBones[i].density;
                if(m < 0.001f) { m = 0.001f; }
                physicsBones[i].body.mass = m;
                totalMass += m;
            }
        }


        protected void DestroySynchronizers()
        {
            for (int i = physicsBones.Length - 1; i >= 0; i--)
            {
                if (!physicsBones[i].synchronizer) { continue; }

                if (Application.isEditor && !Application.isPlaying)
                {
                    DestroyImmediate(physicsBones[i].synchronizer.gameObject);
                }
                else
                {
                    Destroy(physicsBones[i].synchronizer.gameObject);
                }
            }
        }



        float boxScalar = 1f, sphereScalar = 1f, capsuleScalar = 0.5f, radiusScale = 0.2f;

        public void ChangeBoneCollider(int id, string newColliderType)
        {
            if (!Application.isEditor) { return; }

            PhysicsBone phyBone = physicsBones[id];
            if (phyBone.colliderType == newColliderType) { return; }

            Transform physicsTransform = phyBone.transform;

            int direction;
            float distance;
            Vector3 endPoint;

            switch (newColliderType)
            {
                case "Box":

                    if (phyBone.collider) { DestroyImmediate(phyBone.collider); }
                    
                    BoxCollider boxCollider = physicsTransform.gameObject.AddComponent<BoxCollider>();
                    Bounds localBounds = new Bounds();
                    if (physicsTransform.childCount == 0)
                    {
                        endPoint = (physicsTransform.position - physicsTransform.parent.position) + physicsTransform.position;
                        CalculateDirection(physicsTransform.InverseTransformPoint(endPoint), out direction, out distance);
                        localBounds.size = new Vector3(distance, distance, distance);
                    }
                    else
                    {
                        foreach (Transform child in physicsTransform)
                        {
                            localBounds.Encapsulate(physicsTransform.InverseTransformPoint(child.position));
                        }
                    }

                    boxCollider.center = localBounds.center;
                    boxCollider.size = localBounds.size * boxScalar;
                    phyBone.collider = boxCollider;

                    break;


                case "Capsule":

                    if (phyBone.collider) { DestroyImmediate(phyBone.collider); }

                    CapsuleCollider capsuleCollider = physicsTransform.gameObject.AddComponent<CapsuleCollider>();

                    //Adapted from Unity's Ragdoll wizard.

                    if (physicsTransform.childCount == 1)
                    {
                        endPoint = physicsTransform.GetChild(0).position;
                        CalculateDirection(physicsTransform.InverseTransformPoint(endPoint), out direction, out distance);
                    }
                    else
                    {
                        endPoint = (physicsTransform.position - physicsTransform.parent.position) + physicsTransform.position;
                        CalculateDirection(physicsTransform.InverseTransformPoint(endPoint), out direction, out distance);

                        if (physicsTransform.childCount > 1)
                        {
                            Bounds bounds = new Bounds();
                            foreach (Transform child in physicsTransform)
                            {
                                bounds.Encapsulate(physicsTransform.InverseTransformPoint(child.position));
                            }

                            if (distance > 0)
                                distance = bounds.max[direction];
                            else
                                distance = bounds.min[direction];
                        }
                    }

                    capsuleCollider.direction = direction;

                    Vector3 center = Vector3.zero;
                    center[direction] = distance * capsuleScalar;
                    capsuleCollider.center = center;
                    capsuleCollider.height = Mathf.Abs(distance);
                    capsuleCollider.radius = Mathf.Abs(distance * radiusScale);

                    phyBone.collider = capsuleCollider;

                    break;


                case "Sphere":

                    if (phyBone.collider) { DestroyImmediate(phyBone.collider); }

                    SphereCollider sphereCollider = physicsTransform.gameObject.AddComponent<SphereCollider>();

                    endPoint = (physicsTransform.position - physicsTransform.parent.position) + physicsTransform.position;
                    CalculateDirection(physicsTransform.InverseTransformPoint(endPoint), out direction, out distance);


                    sphereCollider.radius = distance * sphereScalar;
                    sphereCollider.center = new Vector3(0, distance, 0);

                    phyBone.collider = sphereCollider;

                    break;

                default:

                    return;
            }
            
            phyBone.colliderType = newColliderType;

        }

        public void ChangeBoneJoint(int id, string newJointType)
        {
            PhysicsBone physicsBone = physicsBones[id];
            if (physicsBone.jointType == newJointType) { return; }

            Transform physicsTransform = physicsBone.transform;
            switch (newJointType)
            {
                case "None":

                    if (physicsBone.joint) { DestroyImmediate(physicsBone.joint); }

                    break;


                case "Hinge":

                    if (!physicsBone.joint) { physicsBone.joint = physicsBone.transform.gameObject.AddComponent<ConfigurableJoint>(); }
                    physicsBone.joint.enablePreprocessing = false;
                    physicsBone.joint.connectedBody = physicsBone.transform.parent.GetComponent<Rigidbody>();

                    physicsBone.joint.xMotion = ConfigurableJointMotion.Locked;
                    physicsBone.joint.yMotion = ConfigurableJointMotion.Locked;
                    physicsBone.joint.zMotion = ConfigurableJointMotion.Locked;
                    physicsBone.joint.angularXMotion = ConfigurableJointMotion.Limited;
                    physicsBone.joint.angularYMotion = ConfigurableJointMotion.Locked;
                    physicsBone.joint.angularZMotion = ConfigurableJointMotion.Locked;

                    var lower = physicsBone.joint.lowAngularXLimit;
                    lower.limit = -10;
                    lower.contactDistance = 2;
                    physicsBone.joint.lowAngularXLimit = lower;

                    var high = physicsBone.joint.highAngularXLimit;
                    high.limit = 95;
                    high.contactDistance = 2;
                    physicsBone.joint.highAngularXLimit = high;
                     
                    SynchronizerSettings syncSettings = physicsBone.synchronizerSettings;

                    physicsBone.joint.rotationDriveMode = RotationDriveMode.Slerp;
                    var slerpDrive = physicsBone.joint.slerpDrive;
                    slerpDrive.maximumForce = syncSettings.rotationMaxForce;
                    slerpDrive.positionDamper = syncSettings.rotationDamper;
                    slerpDrive.positionSpring = syncSettings.rotationSpring;
                    physicsBone.joint.slerpDrive = slerpDrive;

                    physicsBone.joint.projectionMode = JointProjectionMode.PositionAndRotation;
                    physicsBone.joint.projectionDistance = 0.01f;
                    physicsBone.joint.projectionAngle = 1;

                    break;


                case "Ball":

                    if (!physicsBone.joint) { physicsBone.joint = physicsBone.transform.gameObject.AddComponent<ConfigurableJoint>(); }
                    physicsBone.joint.enablePreprocessing = false;
                    physicsBone.joint.connectedBody = physicsBone.transform.parent.GetComponent<Rigidbody>();

                    physicsBone.joint.xMotion = ConfigurableJointMotion.Locked;
                    physicsBone.joint.yMotion = ConfigurableJointMotion.Locked;
                    physicsBone.joint.zMotion = ConfigurableJointMotion.Locked;
                    physicsBone.joint.angularXMotion = ConfigurableJointMotion.Limited;
                    physicsBone.joint.angularYMotion = ConfigurableJointMotion.Limited;
                    physicsBone.joint.angularZMotion = ConfigurableJointMotion.Limited;

                    var lowerX = physicsBone.joint.lowAngularXLimit;
                    lowerX.limit = -10;
                    lowerX.contactDistance = 2;
                    physicsBone.joint.lowAngularXLimit = lowerX;

                    var highX = physicsBone.joint.highAngularXLimit;
                    highX.limit = 95;
                    highX.contactDistance = 2;
                    physicsBone.joint.highAngularXLimit = highX;

                    var angleZ = physicsBone.joint.angularZLimit;
                    angleZ.limit = 30;
                    angleZ.contactDistance = 2;
                    physicsBone.joint.angularZLimit = angleZ;

                    var angleY = physicsBone.joint.angularYLimit;
                    angleY.limit = 45;
                    angleY.contactDistance = 2;
                    physicsBone.joint.angularYLimit = angleY;

                    physicsBone.joint.rotationDriveMode = RotationDriveMode.Slerp;
                    var slerpD = physicsBone.joint.slerpDrive;
                    slerpD.maximumForce = float.PositiveInfinity;
                    slerpD.positionDamper = 100;
                    slerpD.positionSpring = 20000;
                    physicsBone.joint.slerpDrive = slerpD;

                    physicsBone.joint.projectionMode = JointProjectionMode.PositionAndRotation;
                    physicsBone.joint.projectionDistance = 0.01f;
                    physicsBone.joint.projectionAngle = 0.1f;
                    break;
                default:return;
            }
            physicsBone.jointType = newJointType;
        }

        public void ApplyPartAToPartB(int a, int b)
        {
            PhysicsBone partA = physicsBones[a],
                        partB = physicsBones[b];

            partB.colliderType = partA.colliderType;
            DestroyImmediate(partB.collider);

            switch (partA.colliderType)
            {
                case "Box":
                    BoxCollider aBox = (BoxCollider)partA.collider;
                    BoxCollider bBox = partB.transform.gameObject.AddComponent<BoxCollider>();
                    partB.collider = bBox;

                    bBox.center = aBox.center;
                    bBox.size = aBox.size;
                    break;

                case "Capsule":
                    CapsuleCollider aCap = (CapsuleCollider)partA.collider;
                    CapsuleCollider bCap = partB.transform.gameObject.AddComponent<CapsuleCollider>();
                    partB.collider = bCap;

                    bCap.direction = aCap.direction;
                    bCap.radius = aCap.radius;
                    bCap.center = aCap.center;
                    bCap.height = aCap.height;
                    break;

                case "Sphere":
                    SphereCollider aSph = (SphereCollider)partA.collider;
                    SphereCollider bSph = partB.transform.gameObject.AddComponent<SphereCollider>();
                    partB.collider = bSph;

                    bSph.radius = aSph.radius;
                    bSph.center = aSph.center;
                    break;
            }

            partB.jointType = partA.jointType;

            if (partA.jointType == "None") { if (partB.joint) DestroyImmediate(partB.joint); }
            else
            {
                if (!partB.joint) { partB.joint = partB.transform.gameObject.AddComponent<ConfigurableJoint>(); }

                partB.joint.connectedBody = partB.transform.parent.GetComponent<Rigidbody>();

                partB.joint.xMotion = partA.joint.xMotion;
                partB.joint.yMotion = partA.joint.yMotion;
                partB.joint.zMotion = partA.joint.zMotion;
                partB.joint.angularXMotion = partA.joint.angularXMotion;
                partB.joint.angularYMotion = partA.joint.angularYMotion;
                partB.joint.angularZMotion = partA.joint.angularZMotion;

                partB.joint.lowAngularXLimit = partA.joint.lowAngularXLimit;
                partB.joint.highAngularXLimit = partA.joint.highAngularXLimit;

                partB.joint.rotationDriveMode = RotationDriveMode.Slerp;
                partB.joint.slerpDrive = partA.joint.slerpDrive;

                partB.joint.projectionMode = partA.joint.projectionMode;
                partB.joint.projectionDistance = partA.joint.projectionDistance;
                partB.joint.projectionAngle = partA.joint.projectionAngle;

            }
        }

        public void CalculateDirection(Vector3 point, out int direction, out float distance)
        {
            // Calculate longest axis
            direction = 0;
            if (Mathf.Abs(point[1]) > Mathf.Abs(point[0]))
                direction = 1;
            if (Mathf.Abs(point[2]) > Mathf.Abs(point[direction]))
                direction = 2;

            distance = point[direction];
        }

        [ContextMenu("Set Joint Values From Hierarchy Depth")]
        public void SetJointValuesFromHierarchyDepth()
        {
            
            for(int i =0; i < physicsRoots.Length; i++)
            {
                var root = physicsRoots[i];
                int rootDepth = root.Depth();
                ConfigurableJoint[] childJoints = root.GetComponentsInChildren<ConfigurableJoint>(true);
                for(int j = 0; j < childJoints.Length; j++)
                {
                    var joint = childJoints[j];
                    var body = joint.gameObject.GetComponent<Rigidbody>();
                    int jointDepth = joint.transform.Depth() - rootDepth;
                    SynchronizerSettings syncSettings;
                    if (jointDepth <= 0)
                    {
                        syncSettings = DefaultSyncSettings;
                    }
                    else
                    {
                        float scalar = 1f / (jointDepth + 1);
                        float dampScalar = jointDepth * 0.5f;
                        syncSettings = new SynchronizerSettings()
                        {
                            positionDamper = DefaultSyncSettings.positionDamper, // * dampScalar,
                            positionMaxForce = DefaultSyncSettings.positionMaxForce,//  * scalar,
                            positionSpring = DefaultSyncSettings.positionSpring, // * scalar,
                            rotationDamper = DefaultSyncSettings.rotationDamper, // * dampScalar,
                            rotationMaxForce = DefaultSyncSettings.rotationMaxForce, // * scalar,
                            rotationSpring = DefaultSyncSettings.rotationSpring, // * scalar
                        };
                    }

                    joint.rotationDriveMode = RotationDriveMode.Slerp;
                    var slerpDrive =joint.slerpDrive;
                    slerpDrive.maximumForce = syncSettings.rotationMaxForce * body.mass;
                    slerpDrive.positionDamper = syncSettings.rotationDamper;
                    slerpDrive.positionSpring = syncSettings.rotationSpring * body.mass;
                    joint.slerpDrive = slerpDrive;


                }
            }
        }
        [ContextMenu("Set Joint Values From Default Settings")]
        public void SetJointValuesFromDefaultSettings()
        {
            for(int i = 0; i < physicsBones.Length; i++)
            {
                ConfigurableJoint joint = physicsBones[i].joint;
                Rigidbody body = physicsBones[i].body;
                SynchronizerSettings syncSettings = DefaultSyncSettings;

                joint.rotationDriveMode = RotationDriveMode.Slerp;
                var slerpDrive = joint.slerpDrive;
                slerpDrive.maximumForce = syncSettings.rotationMaxForce * body.mass;
                slerpDrive.positionDamper = syncSettings.rotationDamper;
                slerpDrive.positionSpring = syncSettings.rotationSpring * body.mass;
                if(slerpDrive.positionSpring < syncSettings.rotationMinForce) { slerpDrive.positionSpring = syncSettings.rotationMinForce; }
                if(slerpDrive.maximumForce < syncSettings.rotationMinForce * 2) { slerpDrive.maximumForce = syncSettings.rotationMinForce * 2; }
                joint.slerpDrive = slerpDrive;

                
                joint = physicsBones[i].synchronizerJoint;
                //body = physicsBones[i].synchronizerBody;

                joint.rotationDriveMode = RotationDriveMode.Slerp;
                var posDrive = joint.xDrive;
                posDrive.maximumForce = syncSettings.positionMaxForce * body.mass * 100;
                posDrive.positionSpring = syncSettings.positionSpring * body.mass * 100;
                posDrive.positionDamper = syncSettings.positionDamper;
                if (posDrive.positionSpring < syncSettings.positionMinForce) { posDrive.positionSpring = syncSettings.positionMinForce; }
                if (posDrive.maximumForce < syncSettings.positionMinForce * 2) { posDrive.maximumForce = syncSettings.positionMinForce * 2; }
                joint.xDrive = posDrive;

                posDrive = joint.yDrive;
                posDrive.maximumForce = syncSettings.positionMaxForce * body.mass * 100;
                posDrive.positionSpring = syncSettings.positionSpring * body.mass * 100;
                posDrive.positionDamper = syncSettings.positionDamper;
                if (posDrive.positionSpring < syncSettings.positionMinForce) { posDrive.positionSpring = syncSettings.positionMinForce; }
                if (posDrive.maximumForce < syncSettings.positionMinForce * 2) { posDrive.maximumForce = syncSettings.positionMinForce * 2; }
                joint.yDrive = posDrive;

                posDrive = joint.zDrive;
                posDrive.maximumForce = syncSettings.positionMaxForce * body.mass * 100;
                posDrive.positionSpring = syncSettings.positionSpring * body.mass * 100;
                posDrive.positionDamper = syncSettings.positionDamper;
                if (posDrive.positionSpring < syncSettings.positionMinForce) { posDrive.positionSpring = syncSettings.positionMinForce; }
                if (posDrive.maximumForce < syncSettings.positionMinForce * 2) { posDrive.maximumForce = syncSettings.positionMinForce * 2; }
                joint.zDrive = posDrive;

                joint.xMotion = joint.yMotion = joint.zMotion = ConfigurableJointMotion.Free;

                slerpDrive = joint.slerpDrive;
                slerpDrive.maximumForce = syncSettings.rotationMaxForce * body.mass*100;
                slerpDrive.positionDamper = syncSettings.rotationDamper;
                slerpDrive.positionSpring = syncSettings.rotationSpring * body.mass*100;
                if (slerpDrive.positionSpring < syncSettings.rotationMinForce) { slerpDrive.positionSpring = syncSettings.rotationMinForce; }
                if (slerpDrive.maximumForce < syncSettings.rotationMinForce * 2) { slerpDrive.maximumForce = syncSettings.rotationMinForce * 2; }
                joint.slerpDrive = slerpDrive;
                
            }

            for(int i =0; i < physicsBones.Length; i++)
            {

            }
        }
    }

    public enum PhysicsAnimationMode
    {
        NotAnimated, MotorDriven, Synchronized
    }

    [Serializable]
    public struct SynchronizerSettings
    {
        public float rotationMinForce;
        public float rotationMaxForce;
        public float rotationSpring;
        public float rotationDamper;

        public float positionMinForce;
        public float positionMaxForce;
        public float positionSpring;
        public float positionDamper;
    }

    [Serializable]
    public class PhysicsBone 
    {
        [SerializeField]
        PhysicsAnimationMode animationMode;

        public PhysicsAnimationMode AnimationMode { get { return animationMode; }
            set
            {
                if(value != animationMode)
                {

                    switch (animationMode)
                    {
                        case PhysicsAnimationMode.NotAnimated:
                            break;

                        case PhysicsAnimationMode.MotorDriven:
                            break;

                        case PhysicsAnimationMode.Synchronized:
                            synchronizer.gameObject.SetActive(false);
                            break;
                    }

                    animationMode = value;

                    switch (animationMode)
                    {
                        case PhysicsAnimationMode.NotAnimated:
                            break;
                        case PhysicsAnimationMode.MotorDriven:
                            break;
                        case PhysicsAnimationMode.Synchronized:
                            synchronizer.gameObject.SetActive(true);
                            break;
                    }
                    OnAnimationModeChange?.Invoke(this);
                }
            }
        }

        public event Action<PhysicsBone> OnAnimationModeChange;

        public SynchronizerSettings synchronizerSettings;
        public Transform transform, synchronizer;
        public Rigidbody body, synchronizerBody;
        public string colliderType, jointType;
        public Collider collider;
        public CollisionSensor collisionSensor;

        public ConfigurableJoint joint, synchronizerJoint;

        public float density, boundsToVolumeScalar = 0.85f;

        public float Volume()
        {
            if (!collider) { return 0; }
            return collider.bounds.Volume() * boundsToVolumeScalar;
        }

        [SerializeField]
        Quaternion initialRotation;
        public Quaternion InitialRotation { get { return initialRotation; } set { initialRotation = value; } }

    }

    [System.Serializable]
    public class PhysicsSkinner
    {
        public SkinnedMeshRenderer skinRenderer;

        public Transform animationRoot, physicsRoot;
        public Transform[] animationBones, physicsBones;
    }

}