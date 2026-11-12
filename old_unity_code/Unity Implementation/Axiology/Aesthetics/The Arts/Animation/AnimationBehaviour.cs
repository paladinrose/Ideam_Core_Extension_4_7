using System.Collections;
using System.Collections.Generic;

using UnityEngine;
using UnityEngine.Animations.Rigging;

using Ideam.Science.Formal.Mathematics.Geometry.Splines;
using System;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Animation
{

    public enum AnimationGroupSymmetry
    {
        Asymmetric, Axial, Radial, Bilateral, Biradial
    }
    
    [Serializable]
    public class AnimationGroup 
    {
        public string name;
        
        public AnimationGroupSymmetry symmetryType;
        public Transform[] bones;
        public int[] boneIDs = new int[0], rigIDs = new int[0];

        public Color color;

    }

    [Serializable]
    public class AnimationPose
    {
        public Spline3 positionSpline;
        public Spline3 rotationSpline;

        public AnimationPose(Vector3[] positionTargets, Vector3[] rotationTargets)
        {
            positionSpline = new Spline3();
            positionSpline.defaultCurveType = CurveType.CatmullRom;
            positionSpline.SetAnchorPoints(positionTargets.ToFloat3());

            rotationSpline = new Spline3();
            rotationSpline.defaultCurveType = CurveType.CatmullRom;
            rotationSpline.SetAnchorPoints(rotationTargets.ToFloat3());
        }
    }

    [Serializable]
    public class AnimationGesture
    {
        public int[] gestureIDs;
        public AnimationGesture(int l)
        {
            gestureIDs = new int[l];
        }

        public int this[int id]
        {
            get
            {
                if (id < 0 || id >= gestureIDs.Length) { return -1; }
                else { return gestureIDs[id]; }
            }
            set
            {
                gestureIDs[id] = value;
            }
        }
    }

    [Serializable]
    public class AnimationRigging
    {
        public Rig rig;
        public int rigID;
        
        public float absoluteWeight, relativeWeight;

        public AnimationGesture[] gestures = new AnimationGesture[0];

        public AnimationControl[] controls = new AnimationControl[0];

        public IRigConstraint[] constraints = new IRigConstraint[0];
        public Component[] constraintComponents = new Component[0];

        public Color rigColor;

        public void Gesture(int id, float percent)
        {
            if (id < 0 || id >= gestures.Length) { return; }

            AnimationGesture gID = gestures[id];
            for(int i =0; i < controls.Length; i++)
            {
                controls[i].Pose(gID[i], percent);
            }
        }

        public void BlendGestures(int gestureA, float percentA, int gestureB, float percentB, float blend)
        {
            if(gestureA < 0 || gestureB < 0 || gestureA >= gestures.Length || gestureB >= gestures.Length) { return; }
            AnimationGesture gA = gestures[gestureA], 
                             gB = gestures[gestureB];
            for(int i = 0; i < controls.Length; i++)
            {
                controls[i].BlendPoses(gA[i], percentA, gB[i], percentB, blend);
            }
        }

        public void ValidateConstraints()
        {
            if (constraints.Length == constraintComponents.Length) return;

            constraints = new IRigConstraint[constraintComponents.Length];
            for(int i =0; i < constraints.Length; i++)
            {
                constraints[i] = (IRigConstraint)constraintComponents[i];
            }
        }
    }

    
    public class AnimationBehaviour : MonoBehaviour
    {
        [SerializeField, Range(0,1)]
        protected float weight;
        public float Weight { get { return weight; } 
            set
            {
                if(value == weight) { return; }

                weight = value;
                
                for(int i = 0; i < rigging.Length; i++)
                {
                    rigging[i].relativeWeight = rigging[i].rig.weight =   rigging[i].absoluteWeight * weight;
                }
            } 
        }

        public Animator riggedAnimator;

        public Transform[] animationRoots = new Transform[0];
        public Transform[] animationBones = new Transform[0];
        
        public RigBuilder rigBuilder;
        public AnimationRigging[] rigging = new AnimationRigging[0];

        public AnimationGesture[] gestures = new AnimationGesture[0];

        public AnimationGroup[] groups = new AnimationGroup[0];

        #region Monobehaviour Methods

        protected virtual void Awake()  
        {
            SetupRigging();
            
        }
        
        protected virtual void OnEnable()
        {
            SetRigWeightMode(false);
        }

        protected virtual void OnDisable()
        {
            SetRigWeightMode(true);
        }

        protected virtual void OnDestroy()
        {
            SetRigWeightMode(true);
        }

        #endregion

        protected virtual void SetupRigging()
        {
            var layers = rigBuilder.layers;
            
            for (int i =0; i < rigging.Length; i++)
            {
                rigging[i].ValidateConstraints();
                rigging[i].absoluteWeight = rigging[i].rig.weight;
                rigging[i].relativeWeight = rigging[i].absoluteWeight * weight;
                    
                for (int l = 0; l < layers.Count; l++)
                { 
                    if (rigging[i].rig == layers[l].rig) { rigging[i].rigID = l; break; }
                }
            }
            
        }

        

        public virtual int[] FindBones(AnimationRiggingProfile.ProfileTerms boneSearch)
        {
            List<int> boneIDs = new List<int>();
            for(int i =0; i <animationBones.Length; i++)
            {
                if (boneSearch.CheckTerm(animationBones[i].name)) { boneIDs.Add(i); }
            }
            return boneIDs.ToArray();
        }

        public virtual int GetBoneID(Transform targetObject)
        {
            for(int i =0; i < animationBones.Length; i++)
            {
                if(animationBones[i] == targetObject) { return i; }
            }
            return -1;
        }

        public virtual int HasGroup(string groupName)
        {
            groupName = groupName.ToLower();

            for (int i = 0; i < groups.Length; i++)
            {
                if (groups[i].name.ToLower() == groupName)
                {
                    return i;
                }
            }
            return -1;
        }

        public virtual AnimationControl Control(int controlID)
        {
            int count = 0;
            for(int r = 0; r < rigging.Length; r++)
            {
                int rCount = count + rigging[r].controls.Length;
                if(rCount < controlID)
                {
                    count = rCount;
                }
                else
                {
                    int cID = controlID - count;
                    return rigging[r].controls[cID];
                }
            }
            return null;
        }

        public virtual AnimationControl[] Controls(Transform target)
        {
            List<AnimationControl> targetControls = new List<AnimationControl>();
            for(int r = 0; r < rigging.Length; r++)
            {
                for(int c = 0; c < rigging[r].controls.Length; c++)
                {
                    AnimationControl control = rigging[r].controls[c];
                    if(control.transform == target)
                    {
                        targetControls.Add(control);
                    }
                }
            }
            return targetControls.ToArray();
        }

        public virtual AnimationControl[] Controls(AnimationRiggingProfile.ProfileTerms searchTerms)
        {
            List<AnimationControl> targetControls = new List<AnimationControl>();
            for (int r = 0; r < rigging.Length; r++)
            {
                for (int c = 0; c < rigging[r].controls.Length; c++)
                {

                    AnimationControl control = rigging[r].controls[c];
                    if (searchTerms.CheckTerm(control.transform.name))
                    {
                        targetControls.Add(control);
                    }
                }
            }
            return targetControls.ToArray();
        }

        public virtual void SetRigWeightMode(bool absolute)
        {
            if (absolute)
            {
                for(int i =0; i < rigging.Length; i++)
                {
                    rigging[i].rig.weight = rigging[i].absoluteWeight;
                }
            }
            else
            {
                for (int i = 0; i < rigging.Length; i++)
                {
                    rigging[i].rig.weight = rigging[i].relativeWeight;
                }
            }
        }

        public virtual void EnableRigs()
        {
            for(int i =0; i < rigging.Length; i++)
            {
                rigBuilder.layers[rigging[i].rigID].active = true;
            }
        }

        public virtual void DisableRigs()
        {
            for (int i = 0; i < rigging.Length; i++)
            {
                rigBuilder.layers[rigging[i].rigID].active = false;
            }
        }

        public virtual void SetConstraintWeight(int rigID, int constraintID, float constraintWeight)
        {

            if(rigID< 0)
            {
                if(rigID == -10)
                {
                    //apply to all rigs.
                    if (constraintID < 0)
                    {
                        if (constraintID == -10)
                        {
                            //apply to all constraints.
                            for (int i = 0; i < rigging.Length; i++)
                            {
                                for (int j = 0; j < rigging[i].constraints.Length; j++)
                                {
                                    rigging[i].constraints[j].weight = constraintWeight;
                                }
                            }
                        }

                        //Invalid constraint.
                        return;
                    }
                    for(int i =0; i < rigging.Length; i++)
                    {
                        if(constraintID <= rigging[i].constraints.Length) { continue; }

                        rigging[i].constraints[constraintID].weight = constraintWeight;
                    }
                }
            }
            else
            {
                if(rigID >= rigging.Length) { return; }

                AnimationRigging currentRig = rigging[rigID];
                if(constraintID < 0)
                {
                    if(constraintID == -10)
                    {
                        //apply to all constraints.
                        for(int j =0; j < currentRig.constraints.Length; j++)
                        {
                            currentRig.constraints[j].weight = constraintWeight;
                        }
                    }
                }else if(constraintID < currentRig.constraints.Length)
                {
                   currentRig.constraints[constraintID].weight = constraintWeight;
                }
            }
        }

        public virtual void UpdateRigControls()
        {
            for(int i =0; i < rigging.Length; i++)
            {
                if (!rigBuilder.layers[rigging[i].rigID].active) { continue; }
                for(int j = 0; j < rigging[i].controls.Length; j++)
                {
                    rigging[i].controls[j].UpdateControl();
                }
            }
        }

        public virtual void Gesture(int id, float percent)
        {

            if (id < 0 || id >= gestures.Length) { return; }

           
            AnimationGesture gID = gestures[id];

            for (int i = 0; i < rigging.Length; i++)
            {
                rigging[i].Gesture(gID[i], percent);
            }
        }

        public virtual void BlendGestures(int gestureA, float percentA, int gestureB, float percentB, float blend)
        {
            if (gestureA < 0 || gestureB < 0 || gestureA >= gestures.Length || gestureB >= gestures.Length) { return; }
            AnimationGesture gA = gestures[gestureA],
                             gB = gestures[gestureB];
            for (int i = 0; i < rigging.Length; i++)
            {
                rigging[i].BlendGestures(gA[i], percentA, gB[i], percentB, blend);
            }
        }

        public static T CreateGenericAnimationRig<T>(Animator toRig, Transform[] animationRoots, AnimationRiggingProfile riggingProfile) where T:AnimationBehaviour
        {

           
            // Using the animator and roots provided, I create the rigging behaviour, then rigs, constraints, and controls for them, using a rigging profile.
            T animatorRigging = toRig.gameObject.GetComponent<T>();
            
            if (!animatorRigging)
            {
                animatorRigging = toRig.gameObject.AddComponent<T>();
            }
            else
            {
                return animatorRigging;
            }
            animatorRigging.riggedAnimator = toRig;
            animatorRigging.weight = 1f;

            List<AnimationRigging> newRigging = new List<AnimationRigging>();
            
            List<RigLayer> rigLayers = new List<RigLayer>();

            RigBuilder rigBuilder = toRig.gameObject.GetComponent<RigBuilder>();
            
            if (!rigBuilder)
            {
                rigBuilder = toRig.gameObject.AddComponent<RigBuilder>();
            }
            else
            {
                rigLayers = rigBuilder.layers;
                for(int i = rigLayers.Count - 1; i >= 0; i--)
                {
                    if(rigLayers[i].rig == null) { rigLayers.RemoveAt(i); }
                }
            }
            animatorRigging.animationRoots = animationRoots;
            animatorRigging.rigBuilder = rigBuilder;

            int rigLayerID = rigLayers.Count;

            GameObject rootControlObject = new GameObject(toRig.name + " Root Control", typeof(RigTransform));
            
            rootControlObject.transform.SetParent(toRig.transform);
            rootControlObject.transform.localEulerAngles = rootControlObject.transform.localPosition = Vector3.zero;

            Transform[] allBones;

            List<Transform> st = new List<Transform>();
            List<int> asymmetricGroups = new List<int>(),
                      axialGroups = new List<int>(),
                      radialGroups = new List<int>(),
                      bilateralLeftGroups = new List<int>(),
                      bilateralRightGroups = new List<int>(),
                      biradialLeftGroups = new List<int>(),
                      biradialRightGroups = new List<int>();

            //Each root gets handled separately, to prevent a lot of constraints from reaching objects that they shouldn't interact with.
            for (int i = 0; i < animationRoots.Length; i++)
            {
                Transform animatedRoot = animationRoots[i];


                // Get all the bones!
                Transform[] animatedBones = animationRoots[i].GetComponentsInChildren<Transform>();
                st.AddRange(animatedBones);

                // For each, check it with our profile.
                for (int j = 0; j < animatedBones.Length; j++)
                {
                    Transform animatedBone = animatedBones[j];

                    string sn = animatedBone.name;
                    if (riggingProfile.asymmetricGroupTerms.CheckTerm(sn))
                    {
                        asymmetricGroups.Add(j);
                    }
                    if (riggingProfile.axialGroupTerms.CheckTerm(sn))
                    {
                        axialGroups.Add(j);
                    }
                    if (riggingProfile.radialGroupTerms.CheckTerm(sn))
                    {
                        radialGroups.Add(j);
                    }
                    if (riggingProfile.bilateralLeftTerms.CheckTerm(sn))
                    {
                        bilateralLeftGroups.Add(j);
                    }
                    if (riggingProfile.bilateralRightTerms.CheckTerm(sn))
                    {
                        bilateralRightGroups.Add(j);
                    }
                    if (riggingProfile.biradialLeftTerms.CheckTerm(sn))
                    {
                        biradialLeftGroups.Add(j);
                    }
                    if (riggingProfile.biradialRightTerms.CheckTerm(sn))
                    {
                        biradialRightGroups.Add(j);
                    }

                    int[] ids = riggingProfile.MatchingEntries(animatedBone.name);

                    // When we get any profile matches, we use each of their info to create a rig, Ccnstraints, and controls
                    for (int k = 0; k < ids.Length; k++)
                    {
                        AnimationRiggingProfile.RigProfileEntry rigEntry = riggingProfile.profileEntries[ids[k]];
                        
                        Transform rco = rootControlObject.transform;
                        
                        if (rigEntry.rootControlTerms.acceptedTerms.Length > 0)
                        {
                            for (int conC = 0; conC < newRigging.Count; conC++)
                            {
                                bool foundRootCon = false;
                                for (int nrcons = 0; nrcons < newRigging[conC].controls.Length; nrcons++)
                                {
                                    if (rigEntry.rootControlTerms.CheckTerm(newRigging[conC].controls[nrcons].transform.name))
                                    {
                                        foundRootCon = true;
                                        rco = newRigging[conC].controls[nrcons].transform;
                                        break;
                                    }
                                }
                                if (foundRootCon) { break; }
                            }
                        }

                        AnimationRigging animRigging = animatorRigging.GenerateRigging(toRig, animatedRoot, animatedBone, rco, riggingProfile, ids[k], newRigging);


                        //EDITOR
                        animRigging.rigColor = rigEntry.color;

                        animRigging.rigID = rigLayerID;
                        rigLayerID++;

                        rigLayers.Add(new RigLayer(animRigging.rig));

                        newRigging.Add(animRigging);
                    }
                }
            }
           
            allBones = st.ToArray();


            List<AnimationGroup> groups = new List<AnimationGroup>();

            AnimationGroup newGroup;

            if (axialGroups.Count > 0)
            {
                newGroup = new AnimationGroup() { symmetryType = AnimationGroupSymmetry.Axial, name = "Axial" };

                newGroup.bones = new Transform[axialGroups.Count];
                newGroup.boneIDs = axialGroups.ToArray();
                for (int i = 0; i < newGroup.bones.Length; i++)
                {
                    newGroup.bones[i] = allBones[axialGroups[i]];
                }
                groups.Add(newGroup);
            }

            if (asymmetricGroups.Count > 0)
            {
                newGroup = new AnimationGroup() { symmetryType = AnimationGroupSymmetry.Asymmetric, name = "Asymmetric" };

                newGroup.bones = new Transform[asymmetricGroups.Count];
                newGroup.boneIDs = asymmetricGroups.ToArray();
                for (int i = 0; i < newGroup.bones.Length; i++)
                {
                    newGroup.bones[i] = allBones[asymmetricGroups[i]];
                }
                groups.Add(newGroup);
            }

            if (radialGroups.Count > 0)
            {
                newGroup = new AnimationGroup() { symmetryType = AnimationGroupSymmetry.Asymmetric, name = "Radial" };

                newGroup.bones = new Transform[radialGroups.Count];
                newGroup.boneIDs = radialGroups.ToArray();
                for (int i = 0; i < newGroup.bones.Length; i++)
                {
                    newGroup.bones[i] = allBones[radialGroups[i]];
                }
                groups.Add(newGroup);
            }


            AnimationGroup leftRightGroup = new AnimationGroup() { symmetryType = AnimationGroupSymmetry.Bilateral, name = "Bilaterial Symmetry"};
            
            st.Clear();
            List<int> stID = new List<int>();

            List<AnimationGroup> allLeftRightPairs = new List<AnimationGroup>();

            // STILL TO DO: I want to trim the bilateral markings (the things I've got hard coded, like "_left") based on the bilaterialTerms. But I need to think through how.
            for (int l = 0; l < bilateralLeftGroups.Count; l++)
            {
                int lID = bilateralLeftGroups[l];

                string lName = allBones[lID].name;//.ToLower();
                lName = riggingProfile.bilateralLeftTerms.TrimTerm(lName);
                //lName = lName.Replace("_l ", ""); lName = lName.Replace("_left", ""); lName = lName.Replace("left_", "");
               // if (lName.EndsWith("_l")) { lName = lName.Substring(0, lName.Length - 2); }

                for (int r = 0; r < bilateralRightGroups.Count; r++)
                {
                    int rID = bilateralRightGroups[r];
                    string rName = allBones[rID].name;//.ToLower();
                    rName = riggingProfile.bilateralRightTerms.TrimTerm(rName);
                    //rName = rName.Replace("_r ", ""); rName = rName.Replace("_right", ""); rName = rName.Replace("right_", "");
                    //if (rName.EndsWith("_r")) { rName = rName.Substring(0, rName.Length - 2); }

                    if (lName == rName)
                    {

                        st.Add(allBones[lID]); st.Add(allBones[rID]);
                        stID.Add(lID); stID.Add(rID);

                        AnimationGroup lrGroup = new AnimationGroup()  { symmetryType = AnimationGroupSymmetry.Bilateral, name = lName, 
                            bones = new Transform[] { allBones[lID], allBones[rID]},
                            boneIDs = new int[] { lID, rID }
                        };
                        
                        allLeftRightPairs.Add(lrGroup);

                        break;
                    }
                }

            }

            if (st.Count > 1)
            {
                leftRightGroup.bones = st.ToArray();
                leftRightGroup.boneIDs = stID.ToArray();
                groups.Add(leftRightGroup);
                groups.AddRange(allLeftRightPairs);
            }


            allLeftRightPairs.Clear();
            leftRightGroup = new AnimationGroup() { symmetryType = AnimationGroupSymmetry.Bilateral, name = "Biradial Symmetry" };

            st.Clear();
            stID.Clear();

            for (int l = 0; l < biradialLeftGroups.Count; l++)
            {
                int lID = biradialLeftGroups[l];
                string lName = allBones[lID].name;//.ToLower();
                lName = riggingProfile.biradialLeftTerms.TrimTerm(lName);
                //lName = lName.Replace("_l ", ""); lName = lName.Replace("_left", ""); lName = lName.Replace("left_", "");
                //if (lName.EndsWith("_l")) { lName = lName.Substring(0, lName.Length - 2); }

                for (int r = 0; r < biradialRightGroups.Count; r++)
                {
                    int rID = biradialRightGroups[r];
                    string rName = allBones[rID].name;//.ToLower();
                    rName = riggingProfile.biradialRightTerms.TrimTerm(rName);
                    //rName = rName.Replace("_r ", ""); rName = rName.Replace("_right", ""); rName = rName.Replace("right_", "");
                    //if (rName.EndsWith("_r")) { rName = rName.Substring(0, rName.Length - 2); }

                    if (lName == rName)
                    {

                        st.Add(allBones[lID]); st.Add(allBones[rID]);
                        stID.Add(lID); stID.Add(rID);

                        AnimationGroup lrGroup = new AnimationGroup()  { symmetryType = AnimationGroupSymmetry.Bilateral, name = lName, 
                            bones = new Transform[] { allBones[lID], allBones[rID] }, 
                            boneIDs = new int[] { lID, rID }
                        };

                        allLeftRightPairs.Add(lrGroup);

                        break;
                    }
                }

            }

            if (st.Count > 1)
            {
                leftRightGroup.bones = st.ToArray();
                leftRightGroup.boneIDs = stID.ToArray();
                groups.Add(leftRightGroup);
                groups.AddRange(allLeftRightPairs);
            }

            animatorRigging.rigging = newRigging.ToArray();
            animatorRigging.groups = groups.ToArray();
            animatorRigging.animationBones = allBones;
            rigBuilder.layers = rigLayers;
            rootControlObject.transform.SetAsLastSibling();

            return animatorRigging;
        }

        protected virtual AnimationRigging GenerateRigging(Animator toRig, Transform animatedRoot, Transform animatedBone, Transform rootControlObject, AnimationRiggingProfile rigProfile, int entryID, List<AnimationRigging> generatedEntries)
        {
            AnimationRiggingProfile.RigProfileEntry rigEntry = rigProfile.profileEntries[entryID];
            AnimationRigging rigging = new AnimationRigging();

            List<Component> constraintComponents = new List<Component>();
            List<IRigConstraint> constraints = new List<IRigConstraint>();
            List<AnimationControl> controls = new List<AnimationControl>();
            List<int> controlParentIDs = new List<int>();
            
            //4. Create a game object to put this rig on.
            GameObject newRigObject = new GameObject( rigEntry.generatedRigName + " (" + animatedBone.name + ")");
            Transform rigTransform = newRigObject.transform;

            Rig newRig = newRigObject.AddComponent<Rig>();
            newRig.weight = rigEntry.generatedRigWeight;

            rigging.rig = newRig;
            rigging.absoluteWeight = newRig.weight;

            rigTransform.SetParent(toRig.transform);
            rigTransform.localEulerAngles = rigTransform.localPosition = Vector3.zero;

            
            //5. Create our control objects first. This way, we can assign them AS we create our constraints, themselves.
            for (int c = 0; c < rigEntry.generatedControls.Length; c++)
            {
                GameObject[] controlObjects;
                string gcn = rigEntry.generatedControls[c].name;
                if (gcn.StartsWith("*")) 
                {
                    controlObjects = new GameObject[animatedBone.childCount]; gcn = gcn.TrimStart('*');
                    for (int gco = 0; gco < controlObjects.Length; gco++)
                    {
                        GameObject controlObject = new GameObject(gcn + " (" + animatedBone.GetChild(gco).name + ")", typeof(RigTransform));
                        AnimationControl control = controlObject.AddComponent<AnimationControl>();
                        RelativeObjectShadow controlShadow = controlObject.AddComponent<RelativeObjectShadow>();
                        control.lockSettings = controlShadow;
                        control.lockSettings.shadowPosition = rigEntry.generatedControls[c].lockPosition;
                        control.lockSettings.shadowRotation = rigEntry.generatedControls[c].lockRotation;
                        controls.Add(control);
                        controlParentIDs.Add(c);
                        
                        controlObjects[gco] = controlObject;
                    }
                }
                else if (gcn.StartsWith("<")) 
                {
                    gcn = gcn.Substring(1);
                    for (int lr = generatedEntries.Count-1; lr>=0; lr--)
                    {
                        AnimationRigging lastRigging = generatedEntries[lr];
                        bool foundControl = false;
                        for (int lc = 0; lc < lastRigging.controls.Length; lc++)
                        {
                            if (lastRigging.controls[lc].transform.name.Contains(gcn))
                            {
                                controls.Add(lastRigging.controls[lc]);
                                controlParentIDs.Add(c);
                                foundControl = true;
                                break;
                            }
                        }
                        if (foundControl) { break; }
                    }
                }
                else 
                {
                    GameObject controlObject = new GameObject(gcn + " " + animatedBone.name, typeof(RigTransform));
                    AnimationControl control = controlObject.AddComponent<AnimationControl>();
                    RelativeObjectShadow controlShadow = controlObject.AddComponent<RelativeObjectShadow>();
                    control.lockSettings = controlShadow;
                    control.lockSettings.shadowPosition = rigEntry.generatedControls[c].lockPosition;
                    control.lockSettings.shadowRotation = rigEntry.generatedControls[c].lockRotation;
                    controls.Add(control);
                    controlParentIDs.Add(c);
                }
            }

            //6. Create the hiearchy for all the controls, starting with rootControlObject at the, well, root.
            for (int c = 0; c < controls.Count; c++)
            {
                int cpid = rigEntry.generatedControls[controlParentIDs[c]].parentID;
                if (cpid < 0)
                {
                    if(cpid == -100) 
                    { continue; }
                    else if (cpid == -10 && c > 0)
                    {
                        controls[c].transform.SetParent(controls[c - 1].transform);
                    }
                    else if (cpid == -20 && c < controls.Count - 1)
                    {
                        controls[c].transform.SetParent(controls[c + 1].transform);
                    }
                    else
                    {
                        controls[c].transform.SetParent(rootControlObject.transform);
                    }
                }
                else
                {
                    controls[c].transform.SetParent(controls[cpid].transform);
                }
            }

            //7. Go through and generate all the constraints, assinging constrained objects and controls as we go.
            for (int c = 0; c < rigEntry.generatedConstraints.Length; c++)
            {
                GameObject[] objectsToConstrain;
                string gcn = rigEntry.generatedConstraints[c].name;
                if (gcn.StartsWith("*")) 
                {
                    gcn = gcn.TrimStart('*');
                    objectsToConstrain = new GameObject[animatedBone.childCount];
                
                    for (int gco = 0; gco < objectsToConstrain.Length; gco++)
                    {
                        Transform kidBone = animatedBone.GetChild(gco);
                        GameObject constraintObject = new GameObject(gcn + " " + kidBone.name);
                        constraintObject.transform.SetParent(rigTransform);
                        constraintObject.transform.localEulerAngles = constraintObject.transform.localPosition = Vector3.zero;

                        objectsToConstrain[gco] = constraintObject;
                        var constr = GenerateConstraint(toRig, constraintObject.transform, animatedRoot, kidBone, c, controlParentIDs, rigEntry, controls);
                        constraints.Add(constr);
                        constraintComponents.Add((Component)constr);

                    }
                }
                else 
                {
                    GameObject constraintObject = new GameObject(gcn + " " + animatedBone.name);
                    constraintObject.transform.SetParent(rigTransform);
                    constraintObject.transform.localEulerAngles = constraintObject.transform.localPosition = Vector3.zero;

                    var constr = GenerateConstraint(toRig, constraintObject.transform, animatedRoot, animatedBone, c, controlParentIDs, rigEntry, controls);
                    constraints.Add(constr);
                    constraintComponents.Add((Component)constr);
                }

            }

            rigging.constraintComponents = constraintComponents.ToArray();
            rigging.constraints = constraints.ToArray();
            rigging.controls = controls.ToArray();

            return rigging;
        }


        protected virtual Transform[] BoneMatches(Transform rootTransform, Transform localTransform, AnimationRiggingProfile.ProfileTerms termsToMatch)
        {
            List<Transform> matches;
            string matchMethod = termsToMatch.acceptedTerms[0];

            if (matchMethod == ">")
            {
                matches = new List<Transform>(localTransform.GetComponentsInChildren<Transform>(true));
            }
            else if (matchMethod == "<") 
            {
                matches = new List<Transform>(localTransform.GetComponentsInParent<Transform>(true));
            }
            else 
            {
                matches = new List<Transform>(rootTransform.GetComponentsInChildren<Transform>(true));
            }
            for(int i = matches.Count-1; i >=0; i--)
            {
                if (!termsToMatch.CheckTerm(matches[i].name)) { matches.RemoveAt(i); }
            }

            return matches.ToArray();
        }

        protected virtual int[] ControlMatches(IRigConstraint constraint, Transform constrainedTransform, List<AnimationControl> controls, AnimationRiggingProfile.ProfileTerms termsToMatch)
        {
            List<int> controlIDs = new List<int>();
            
            for(int i =0; i < controls.Count; i++)
            {
                if (termsToMatch.CheckTerm(controls[i].transform.name)) { controlIDs.Add(i); }
            }
            if(termsToMatch.acceptedTerms[0] == "*")
            {
                string conName = constrainedTransform.name;

                for(int i = controlIDs.Count - 1; i >= 0; i--)
                {
                    if(!termsToMatch.CompareTerms(controls[controlIDs[i]].transform.name, conName))
                    {
                        controlIDs.RemoveAt(i);
                    }
                }
            }
            return controlIDs.ToArray();
        }


        #region Constraints

        protected virtual IRigConstraint GenerateConstraint(Animator toRig, Transform constraintParent, Transform animatedRoot, Transform animatedBone, int c, List<int> controlEntryIDs,
                                                                          AnimationRiggingProfile.RigProfileEntry rigEntry, List<AnimationControl> controls)
        {
            string cT = rigEntry.generatedConstraints[c].Type;

            if (cT == "TwoBoneIKConstraint")
            {
                TwoBoneIKConstraint twoBoneIKConstraint = GenerateTwoBoneIKConstraint(toRig, constraintParent, animatedRoot, animatedBone, c, controlEntryIDs, rigEntry, controls);
                return twoBoneIKConstraint;
            }
            else if (cT == "BlendConstraint")
            {
                BlendConstraint blendConstraint = GenerateBlendConstraint(toRig, constraintParent, animatedRoot, animatedBone, c, controlEntryIDs, rigEntry, controls);
                return blendConstraint;
            }
            else if (cT == "MultiAimConstraint")
            {
                MultiAimConstraint multiAimConstraint = GenerateMultiAimConstraint(toRig, constraintParent, animatedRoot, animatedBone, c, controlEntryIDs, rigEntry, controls);
                return multiAimConstraint;
            }
            else if (cT == "MultiParentConstraint")
            {
                MultiParentConstraint multiParentConstraint = GenerateMultiParentConstraint(toRig, constraintParent, animatedRoot, animatedBone, c, controlEntryIDs, rigEntry, controls);
                return multiParentConstraint;
            }
            else if (cT == "MultiPositionConstraint")
            {
                MultiPositionConstraint multiPositionConstraint = GenerateMultiPositionConstraint(toRig, constraintParent, animatedRoot, animatedBone, c, controlEntryIDs, rigEntry, controls);
               return multiPositionConstraint;
            }
            else if (cT == "MultiRotationConstraint")
            {
                MultiRotationConstraint multiRotationConstraint = GenerateMultiRotationConstraint(toRig, constraintParent, animatedRoot, animatedBone, c, controlEntryIDs, rigEntry, controls);
                return multiRotationConstraint;
            }
            else if (cT == "MultiReferentialConstraint")
            {
                MultiReferentialConstraint multiReferentialConstraint = GenerateMultiReferentialConstraint(toRig, constraintParent, animatedRoot, animatedBone, c, controlEntryIDs, rigEntry, controls);
                return multiReferentialConstraint;
            }
            else if (cT == "ChainIKConstraint")
            {
                ChainIKConstraint chainIKConstraint = GenerateChainIKConstraint(toRig, constraintParent, animatedRoot, animatedBone, c, controlEntryIDs, rigEntry, controls);
                return chainIKConstraint;
            }
            else if (cT == "TwistChainConstraint")
            {
                TwistChainConstraint twistChainConstraint = GenerateTwistChainConstraint(toRig, constraintParent, animatedRoot, animatedBone, c, controlEntryIDs, rigEntry, controls);
                return twistChainConstraint;
            }
            else if (cT == "TwistCorrection")
            {
                TwistCorrection twistCorrection = GenerateTwistCorrection(toRig, constraintParent, animatedRoot, animatedBone, c, controlEntryIDs, rigEntry, controls);
                return twistCorrection;
            }
            else if (cT == "DampedTransform")
            {
                DampedTransform dampedTransform = GenerateDampedTransform(toRig, constraintParent, animatedRoot, animatedBone, c, controlEntryIDs, rigEntry, controls);
                return dampedTransform;
            } 
            else if (cT == "OverrideTransform")
            {
                OverrideTransform overrideTransform = GenerateOverrideTransform(toRig, constraintParent, animatedRoot, animatedBone, c, controlEntryIDs, rigEntry, controls);
                return overrideTransform;
            }

            return default;
        }

        protected virtual TwoBoneIKConstraint GenerateTwoBoneIKConstraint(Animator toRig, Transform constraintObject, Transform animatedRoot, Transform animatedBone, int constraintID, List<int> controlEntryIDs,
                                                                          AnimationRiggingProfile.RigProfileEntry rigEntry,  List<AnimationControl> controls)
        {
            
            TwoBoneIKConstraint twoBoneIKConstraint = constraintObject.gameObject.AddComponent<TwoBoneIKConstraint>();
            

            AnimationRiggingProfile.ConstraintProfile cProfile = rigEntry.generatedConstraints[constraintID]; 
            Transform[] matchSticks;
            Transform root = null, mid = null, tip = null, ikTarget = null, ikHint = null;

            int[] controlMatches;
            int ikTID = -1, ikHID = -1;
            if (cProfile.boneMappings.Length > 0)
            {
                matchSticks = BoneMatches(animatedRoot, animatedBone, cProfile.boneMappings[0]);

                switch (matchSticks.Length)
                {
                    case 1:
                        root = matchSticks[0];
                        break;
                    case 2:
                        root = matchSticks[0];
                        mid = matchSticks[1];
                        break;
                    case 3:
                        root = matchSticks[0];
                        mid = matchSticks[1];
                        tip = matchSticks[2];
                        break;
                }
                
                if(!mid && cProfile.boneMappings.Length > 1)
                {
                    matchSticks = BoneMatches(animatedRoot, animatedBone, cProfile.boneMappings[1]);
                    if(matchSticks.Length > 0)
                    {
                        mid = matchSticks[0];
                        if(!tip && matchSticks.Length > 1) { tip = matchSticks[1]; }
                    }

                    if (!tip && cProfile.boneMappings.Length > 2)
                    {
                        matchSticks = BoneMatches(animatedRoot, animatedBone, cProfile.boneMappings[2]);
                        if (matchSticks.Length > 0) { tip = matchSticks[0]; }
                    }
                }
                else if (!tip && cProfile.boneMappings.Length > 1) 
                {
                    matchSticks = BoneMatches(animatedRoot, animatedBone, cProfile.boneMappings[1]);
                    if (matchSticks.Length > 0) { tip = matchSticks[0]; }
                }
            }

            if (cProfile.controlMappings.Length > 0)
            {
                controlMatches = ControlMatches(twoBoneIKConstraint, animatedBone, controls, cProfile.controlMappings[0]);

                switch (controlMatches.Length)
                {
                    case 1:
                        ikTID = controlMatches[0];
                        ikTarget = controls[ikTID].transform;
                        break;

                    case 2:
                        ikTID = controlMatches[0];
                        ikHID = controlMatches[1];
                        ikTarget = controls[ikTID].transform;
                        ikHint = controls[ikHID].transform;
                        break;

                    default: break;
                }
                
                if (!ikHint && cProfile.controlMappings.Length > 1)
                {
                    controlMatches = ControlMatches(twoBoneIKConstraint, animatedBone, controls, cProfile.controlMappings[1]);
                    if (controlMatches.Length > 0)
                    {
                        ikHID = controlMatches[0];
                        ikHint = controls[ikHID].transform;
                    }
                }
            }

            
            //Set the constrained bones from animatedBone.

            twoBoneIKConstraint.data.root = root;
            twoBoneIKConstraint.data.mid = mid; 
            twoBoneIKConstraint.data.tip = tip;
            twoBoneIKConstraint.data.target = ikTarget;
            twoBoneIKConstraint.data.hint = ikHint;
            
            if (ikTarget && tip) 
            { 
                ikTarget.MoveIndependent(twoBoneIKConstraint.data.tip.TransformPoint(rigEntry.generatedControls[controlEntryIDs[ikTID]].position));
                ikTarget.RotateIndependent(twoBoneIKConstraint.data.tip.rotation * Quaternion.Euler(rigEntry.generatedControls[controlEntryIDs[ikTID]].rotation));
            }

            if (ikHint && mid && tip) 
            { 
                Vector3 offsetMid = twoBoneIKConstraint.data.mid.TransformPoint(rigEntry.generatedControls[controlEntryIDs[ikHID]].position);
                Vector3 tipToMid = (twoBoneIKConstraint.data.tip.position - offsetMid).normalized;
                Vector3 rootToMid = (twoBoneIKConstraint.data.root.position - offsetMid).normalized;
                Vector3 hintDirection = ((tipToMid + rootToMid) * 0.5f); //.normalized;

                ikHint.MoveIndependent(twoBoneIKConstraint.data.mid.position - (hintDirection));
                ikHint.RotateIndependent(twoBoneIKConstraint.data.mid.rotation * Quaternion.Euler(rigEntry.generatedControls[controlEntryIDs[ikHID]].rotation));
            }
            return twoBoneIKConstraint;
        }

        protected virtual BlendConstraint GenerateBlendConstraint(Animator toRig, Transform constraintObject, Transform animatedRoot, Transform animatedBone, int constraintID, List<int> controlEntryIDs,
                                                                  AnimationRiggingProfile.RigProfileEntry rigEntry, List<AnimationControl> controls)
        {
            AnimationRiggingProfile.ConstraintProfile cProfile = rigEntry.generatedConstraints[constraintID];
          

            BlendConstraint blendConstraint = constraintObject.gameObject.AddComponent<BlendConstraint>();

            //Set the constrained bones from animatedBone.
            blendConstraint.data.sourceObjectA = animatedBone;

            //Set the constraint controls from rigEntry, then align them.
            int[] sbid = ControlMatches(blendConstraint, animatedBone, controls, cProfile.controlMappings[0]);
            if(sbid.Length > 0) 
            {
                Transform sourceB = controls[sbid[0]].transform;
                blendConstraint.data.sourceObjectB = sourceB; 
                sourceB.MoveIndependent(animatedBone.TransformPoint(rigEntry.generatedControls[controlEntryIDs[sbid[0]]].position));
                sourceB.RotateIndependent( animatedBone.rotation * Quaternion.Euler(rigEntry.generatedControls[controlEntryIDs[sbid[0]]].rotation));
            }
            
            return blendConstraint;
        }

        protected virtual MultiParentConstraint GenerateMultiParentConstraint(Animator toRig, Transform constraintObject, Transform animatedRoot, Transform animatedBone, int constraintID, List<int> controlEntryIDs,
                                                                  AnimationRiggingProfile.RigProfileEntry rigEntry, List<AnimationControl> controls)
        {
            AnimationRiggingProfile.ConstraintProfile cProfile = rigEntry.generatedConstraints[constraintID];

            MultiParentConstraint multiParentConstraint = constraintObject.gameObject.AddComponent<MultiParentConstraint>();

            //Set the constrained bones from animatedBone.
            multiParentConstraint.data.constrainedObject = animatedBone;

            //Set the constraint controls from rigEntry, then align them.
            var sourceObjects = multiParentConstraint.data.sourceObjects;
            for (int s = 0; s < cProfile.controlMappings.Length; s++)
            {
                int[] cmap = ControlMatches(multiParentConstraint, animatedBone, controls, cProfile.controlMappings[s]);
                for (int ss = 0; ss < cmap.Length; ss++)
                {
                    int cid = cmap[ss];
                    Transform sT = controls[cid].transform;
                    sT.MoveIndependent(animatedBone.TransformPoint(rigEntry.generatedControls[controlEntryIDs[cid]].position));
                    sT.RotateIndependent(animatedBone.rotation * Quaternion.Euler(rigEntry.generatedControls[controlEntryIDs[cid]].rotation));

                    sourceObjects.Add(new WeightedTransform(sT, 1f));
                }
            }
            multiParentConstraint.data.sourceObjects = sourceObjects;

            return multiParentConstraint;
        }

        protected virtual MultiPositionConstraint GenerateMultiPositionConstraint(Animator toRig, Transform constraintObject, Transform animatedRoot, Transform animatedBone, int constraintID, List<int> controlEntryIDs,
                                                                 AnimationRiggingProfile.RigProfileEntry rigEntry, List<AnimationControl> controls)
        {
            AnimationRiggingProfile.ConstraintProfile cProfile = rigEntry.generatedConstraints[constraintID];
            MultiPositionConstraint multiPositionConstraint = constraintObject.gameObject.AddComponent<MultiPositionConstraint>();


            //Set the constrained bones from animatedBone.
            multiPositionConstraint.data.constrainedObject = animatedBone;

            //Set the constraint controls from rigEntry, then align them.
            var sourceObjects = multiPositionConstraint.data.sourceObjects;
            for (int s = 0; s < cProfile.controlMappings.Length; s++)
            {
                int[] cmap = ControlMatches(multiPositionConstraint, animatedBone, controls, cProfile.controlMappings[s]);
                for (int ss = 0; ss < cmap.Length; ss++)
                {
                    int cid = cmap[ss];
                    Transform sT = controls[cid].transform;
                    sT.MoveIndependent(animatedBone.TransformPoint(rigEntry.generatedControls[controlEntryIDs[cid]].position));
                    sT.RotateIndependent(animatedBone.rotation * Quaternion.Euler(rigEntry.generatedControls[controlEntryIDs[cid]].rotation));

                    sourceObjects.Add(new WeightedTransform(sT, 1f));
                }
            }
            multiPositionConstraint.data.sourceObjects = sourceObjects;

            return multiPositionConstraint;
        }

        protected virtual MultiAimConstraint GenerateMultiAimConstraint(Animator toRig, Transform constraintObject, Transform animatedRoot, Transform animatedBone, int constraintID, List<int> controlEntryIDs,
                                                                 AnimationRiggingProfile.RigProfileEntry rigEntry, List<AnimationControl> controls)
        {
            AnimationRiggingProfile.ConstraintProfile cProfile = rigEntry.generatedConstraints[constraintID];

            MultiAimConstraint multiAimConstraint = constraintObject.gameObject.AddComponent<MultiAimConstraint>();

            //Set the constrained bones from animatedBone.
            multiAimConstraint.data.constrainedObject = animatedBone;
            int ss = 0;
            if (cProfile.boneMappings.Length > 0)
            {
                Transform[] matchSticks = BoneMatches(animatedRoot, animatedBone, cProfile.boneMappings[0]);
                if (matchSticks.Length > 0) { multiAimConstraint.data.worldUpObject = matchSticks[0]; }
            }
            else if (cProfile.controlMappings.Length > 0)
            {
                ss = 1;
                int[] cmap = ControlMatches(multiAimConstraint, animatedBone, controls, cProfile.controlMappings[0]);
                if (cmap.Length > 0)
                {
                    Transform sT = controls[cmap[0]].transform;

                    sT.MoveIndependent(animatedBone.TransformPoint(rigEntry.generatedControls[controlEntryIDs[cmap[0]]].position));
                    sT.RotateIndependent(animatedBone.rotation * Quaternion.Euler(rigEntry.generatedControls[controlEntryIDs[cmap[0]]].rotation));

                    multiAimConstraint.data.worldUpObject = sT;
                }
            }

            if (multiAimConstraint.data.worldUpObject != null) { multiAimConstraint.data.worldUpType = MultiAimConstraintData.WorldUpType.ObjectUp; }
            else { multiAimConstraint.data.worldUpType = MultiAimConstraintData.WorldUpType.SceneUp; }

            //Set the constraint controls from rigEntry, then align them.
            var sourceObjects = multiAimConstraint.data.sourceObjects;
            for (int s = ss; s < cProfile.controlMappings.Length; s++)
            {
                int[] cmap = ControlMatches(multiAimConstraint, animatedBone, controls, cProfile.controlMappings[s]);
                for(int cm = 0; cm < cmap.Length; cm++) 
                {
                    int cid = cmap[cm];
                    Transform sT = controls[cid].transform;
                    sT.MoveIndependent(animatedBone.TransformPoint(rigEntry.generatedControls[controlEntryIDs[cid]].position));
                    sT.RotateIndependent( animatedBone.rotation * Quaternion.Euler(rigEntry.generatedControls[controlEntryIDs[cid]].rotation));

                    sourceObjects.Add(new WeightedTransform(sT, 1f));
                }
            }
            multiAimConstraint.data.sourceObjects = sourceObjects;

            return multiAimConstraint;
        }

        protected virtual MultiRotationConstraint GenerateMultiRotationConstraint(Animator toRig, Transform constraintObject, Transform animatedRoot, Transform animatedBone, int constraintID, List<int> controlEntryIDs,
                                                                 AnimationRiggingProfile.RigProfileEntry rigEntry, List<AnimationControl> controls)
        {
            AnimationRiggingProfile.ConstraintProfile cProfile = rigEntry.generatedConstraints[constraintID];

            MultiRotationConstraint multiRotationConstraint = constraintObject.gameObject.AddComponent<MultiRotationConstraint>();

            //Set the constrained bones from animatedBone.
            multiRotationConstraint.data.constrainedObject = animatedBone;

            //Set the constraint controls from rigEntry, then align them.
            var sourceObjects = multiRotationConstraint.data.sourceObjects;
            for (int s = 0; s < cProfile.controlMappings.Length; s++)
            {
                int[] cmap = ControlMatches(multiRotationConstraint, animatedBone, controls, cProfile.controlMappings[s]);
                for (int ss = 0; ss < cmap.Length; ss++)
                {
                    int cid = cmap[ss];
                    Transform sT = controls[cid].transform;
                    sT.MoveIndependent(animatedBone.TransformPoint(rigEntry.generatedControls[controlEntryIDs[cid]].position));
                    sT.RotateIndependent(animatedBone.rotation * Quaternion.Euler(rigEntry.generatedControls[controlEntryIDs[cid]].rotation));

                    sourceObjects.Add(new WeightedTransform(sT, 1f));
                }
            }
            multiRotationConstraint.data.sourceObjects = sourceObjects;

            return multiRotationConstraint;
        }

        protected virtual MultiReferentialConstraint GenerateMultiReferentialConstraint(Animator toRig, Transform constraintObject, Transform animatedRoot, Transform animatedBone, int constraintID, List<int> controlEntryIDs,
                                                                 AnimationRiggingProfile.RigProfileEntry rigEntry, List<AnimationControl> controls)
        {
            AnimationRiggingProfile.ConstraintProfile cProfile = rigEntry.generatedConstraints[constraintID];

            MultiReferentialConstraint multiReferentialConstraint = constraintObject.gameObject.AddComponent<MultiReferentialConstraint>();


            //Set the constrained bones from animatedBone.
            var sourceObjects = multiReferentialConstraint.data.sourceObjects;
            sourceObjects.Add(animatedBone);
            for (int s = 0; s < cProfile.boneMappings.Length; s++)
            {
                Transform[] matchingBones = BoneMatches(animatedRoot, animatedBone, cProfile.boneMappings[s]);
                for(int mb = 0; mb < matchingBones.Length; mb++)
                {
                    sourceObjects.Add(matchingBones[mb]); 
                }
            }

            //Set the constraint controls from rigEntry, then align them.
            for (int s = 0; s < cProfile.controlMappings.Length; s++)
            {
                int[] cmap = ControlMatches(multiReferentialConstraint, animatedBone, controls, cProfile.controlMappings[s]);
                for (int ss = 0; ss < cmap.Length; ss++)
                {
                    int cid = cmap[ss];
                    Transform sT = controls[cid].transform;
                    sT.MoveIndependent(animatedBone.TransformPoint(rigEntry.generatedControls[controlEntryIDs[cid]].position));
                    sT.RotateIndependent(animatedBone.rotation * Quaternion.Euler(rigEntry.generatedControls[controlEntryIDs[cid]].rotation));

                    sourceObjects.Add(sT);
                }
            }
            multiReferentialConstraint.data.sourceObjects = sourceObjects;
            multiReferentialConstraint.data.driver = 1;

            return multiReferentialConstraint;
        }

        protected virtual ChainIKConstraint GenerateChainIKConstraint(Animator toRig, Transform constraintObject, Transform animatedRoot, Transform animatedBone, int constraintID, List<int> controlEntryIDs,
                                                                 AnimationRiggingProfile.RigProfileEntry rigEntry, List<AnimationControl> controls)
        {
            AnimationRiggingProfile.ConstraintProfile cProfile = rigEntry.generatedConstraints[constraintID];

            ChainIKConstraint chainIKConstraint = constraintObject.gameObject.AddComponent<ChainIKConstraint>();


            if (cProfile.boneMappings.Length > 0)
            {
                Transform[] matchedBones = BoneMatches(animatedRoot, animatedBone, cProfile.boneMappings[0]);
                if (matchedBones.Length > 0)
                {
                    chainIKConstraint.data.root = matchedBones[0];
                    if (matchedBones.Length > 1)
                    {
                        chainIKConstraint.data.tip = matchedBones[1];
                    }
                }

                if (!chainIKConstraint.data.tip && cProfile.boneMappings.Length > 1)
                {
                    matchedBones = BoneMatches(animatedRoot, animatedBone, cProfile.boneMappings[1]);
                    if (matchedBones.Length > 0)
                    {
                        chainIKConstraint.data.tip = matchedBones[0];
                    }
                }
            }


            if (cProfile.controlMappings.Length > 0)
            {
                //Set the constraint controls from rigEntry, then align them.
                int[] chid = ControlMatches(chainIKConstraint, animatedBone, controls, cProfile.controlMappings[0]);
                if (chid.Length > 0)
                {
                    int cid = chid[0];
                    chainIKConstraint.data.target = controls[cid].transform;
                    chainIKConstraint.data.target.MoveIndependent(chainIKConstraint.data.tip.TransformPoint(rigEntry.generatedControls[controlEntryIDs[cid]].position));
                    chainIKConstraint.data.target.RotateIndependent( chainIKConstraint.data.tip.rotation * Quaternion.Euler(rigEntry.generatedControls[controlEntryIDs[cid]].rotation));
                }
            }
            return chainIKConstraint;
        }

        protected virtual TwistChainConstraint GenerateTwistChainConstraint(Animator toRig, Transform constraintObject, Transform animatedRoot, Transform animatedBone, int constraintID, List<int> controlEntryIDs,
                                                                 AnimationRiggingProfile.RigProfileEntry rigEntry, List<AnimationControl> controls)
        {
            AnimationRiggingProfile.ConstraintProfile cProfile = rigEntry.generatedConstraints[constraintID];


            TwistChainConstraint twistChainConstraint = constraintObject.gameObject.AddComponent<TwistChainConstraint>();

            //Set the constrained bones from animatedBone.
            twistChainConstraint.data.tip = animatedBone;
            if (cProfile.boneMappings.Length > 0)
            {
                Transform[] matchedBones = BoneMatches(animatedRoot, animatedBone, cProfile.boneMappings[0]);
                if (matchedBones.Length > 0)
                {
                    twistChainConstraint.data.root = matchedBones[0];
                }
            }

            if (cProfile.controlMappings.Length > 0)
            {
                //Set the constraint controls from rigEntry, then align them.
                int[] cmap = ControlMatches(twistChainConstraint, animatedBone, controls, cProfile.controlMappings[0]);

                if (cmap.Length > 0)
                {
                    int cm = cmap[0];
                    Transform sT = controls[cm].transform;
                    sT.MoveIndependent(animatedBone.TransformPoint(rigEntry.generatedControls[controlEntryIDs[cm]].position));
                    sT.RotateIndependent(animatedBone.rotation * Quaternion.Euler(rigEntry.generatedControls[controlEntryIDs[cm]].rotation));

                    twistChainConstraint.data.tipTarget = sT;

                    
                }

                if (twistChainConstraint.data.root && cProfile.controlMappings.Length > 1)
                {
                    cmap = ControlMatches(twistChainConstraint, animatedBone, controls, cProfile.controlMappings[1]);

                    if (cmap.Length > 0)
                    {
                        int cm = cmap[0];
                        Transform sT = controls[cm].transform;
                        sT.MoveIndependent(twistChainConstraint.data.root.TransformPoint(rigEntry.generatedControls[controlEntryIDs[cm]].position));
                        sT.RotateIndependent(twistChainConstraint.data.root.rotation * Quaternion.Euler(rigEntry.generatedControls[controlEntryIDs[cm]].rotation));

                        twistChainConstraint.data.rootTarget = sT;


                    }
                }
            }

            return twistChainConstraint;
        }

        protected virtual TwistCorrection GenerateTwistCorrection(Animator toRig, Transform constraintObject, Transform animatedRoot, Transform animatedBone, int constraintID, List<int> controlEntryIDs,
                                                                 AnimationRiggingProfile.RigProfileEntry rigEntry, List<AnimationControl> controls)
        {
            AnimationRiggingProfile.ConstraintProfile cProfile = rigEntry.generatedConstraints[constraintID];

            TwistCorrection twistCorrection = constraintObject.gameObject.AddComponent<TwistCorrection>();

            //Set the constrained bones from animatedBone.
            var twistNodes = twistCorrection.data.twistNodes;
            twistNodes.Add(new WeightedTransform(animatedBone, 1f));
            for (int s = 0; s < cProfile.boneMappings.Length; s++)
            {
                Transform[] matchedBones = BoneMatches(animatedRoot, animatedBone, cProfile.boneMappings[0]);
                for(int tn = 0; tn < matchedBones.Length; tn++) { twistNodes.Add(new WeightedTransform(matchedBones[tn], 1f)); }
            }
            twistCorrection.data.twistNodes = twistNodes;

            //Set the constraint controls from rigEntry, then align them.
            if (cProfile.controlMappings.Length > 0)
            {
                int[] sid = ControlMatches(twistCorrection, animatedBone, controls, cProfile.controlMappings[0]);
                Transform source = controls[sid[0]].transform;
                twistCorrection.data.sourceObject = source;
                source.MoveIndependent(animatedBone.TransformPoint(rigEntry.generatedControls[controlEntryIDs[sid[0]]].position));
                source.RotateIndependent(animatedBone.rotation * Quaternion.Euler(rigEntry.generatedControls[controlEntryIDs[sid[0]]].rotation));
            }
            return twistCorrection;
        }

        protected virtual DampedTransform GenerateDampedTransform(Animator toRig, Transform constraintObject, Transform animatedRoot, Transform animatedBone, int constraintID, List<int> controlEntryIDs,
                                                                 AnimationRiggingProfile.RigProfileEntry rigEntry, List<AnimationControl> controls)
        {
            AnimationRiggingProfile.ConstraintProfile cProfile = rigEntry.generatedConstraints[constraintID];

            DampedTransform dampedTransform = constraintObject.gameObject.AddComponent<DampedTransform>();

            //Set the constrained bones from animatedBone.
            dampedTransform.data.constrainedObject = animatedBone;
            //Set the constraint controls from rigEntry, then align them.
            if (cProfile.controlMappings.Length > 0)
            {
                int[] did = ControlMatches(dampedTransform, animatedBone, controls, cProfile.controlMappings[0]);
                Transform source = controls[did[0]].transform;
                dampedTransform.data.sourceObject = source;
                source.MoveIndependent(dampedTransform.data.constrainedObject.TransformPoint(rigEntry.generatedControls[controlEntryIDs[did[0]]].position));
                source.RotateIndependent(dampedTransform.data.constrainedObject.rotation * Quaternion.Euler(rigEntry.generatedControls[controlEntryIDs[did[0]]].rotation));
            }
            return dampedTransform;
        }

        protected virtual OverrideTransform GenerateOverrideTransform(Animator toRig, Transform constraintObject, Transform animatedRoot, Transform animatedBone, int constraintID, List<int> controlEntryIDs,
                                                                 AnimationRiggingProfile.RigProfileEntry rigEntry, List<AnimationControl> controls)
        {
            AnimationRiggingProfile.ConstraintProfile cProfile = rigEntry.generatedConstraints[constraintID];

            OverrideTransform overrideTransform = constraintObject.gameObject.AddComponent<OverrideTransform>();

            //Set the constrained bones from animatedBone.
            overrideTransform.data.constrainedObject = animatedBone;
            //Set the constraint controls from rigEntry, then align them.
            if (cProfile.controlMappings.Length > 0)
            {
                int[] oid = ControlMatches(overrideTransform, animatedBone, controls, cProfile.controlMappings[0]);
                Transform source = controls[oid[0]].transform;
                overrideTransform.data.sourceObject = source;
                source.MoveIndependent(overrideTransform.data.constrainedObject.TransformPoint(rigEntry.generatedControls[controlEntryIDs[oid[0]]].position));
                source.RotateIndependent( overrideTransform.data.constrainedObject.rotation * Quaternion.Euler(rigEntry.generatedControls[controlEntryIDs[oid[0]]].rotation));
            }
            return overrideTransform;
        }

#endregion
    }
}