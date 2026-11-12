using UnityEngine;
using System.Collections;
using System.Collections.Generic;

#if UNITY_EDITOR
using UnityEditor;
#endif

namespace Ideam.Logic.Sets 
{

    public enum HierarchyStatus
    {
        Active,                 // Standard state. Member is active and ready. Disabled if any ancestor is disabled.
        PersistentlyActive,     // Member is active regardless of ancestors.
        Disabled,               // Member and all Active descendents are disabled.
        Paused                  // While not Active, it's not Disabled, and none of its children are causally disabled
    }

    public interface IHierarchyItem<T>
    {
        T Contents { get; }
        Genealogy Genealogy { get; }
        HierarchyStatus Status { get; }

    }

    public interface IHierarchy<T, TContent> where T: IHierarchyItem<TContent>
    {
        
        TContent RootObject { get; }
        int[] RootChildren { get; }
        HierarchyStatus RootStatus { get; }

        TContent this[int index] { get; set; }
        int Length { get; }
        int YoungestGeneration { get; }

        void GetInHierarchicalOrder(out TContent[] hierarchyOrder, out int[] orderedIDs);

        void Add(TContent kid, int parentID = -1);
        //void Add(int id = -1);
        int[] Remove(int id, bool includeKids = true);
        int[] Remove(int[] ids, bool includeKids = true);
        int HasInHierarchy(TContent check);

        void SetAsRoot(int id);
        void ReplaceParent(int id, int newParent);
        int[] GetAllDescendants(int id);
        
        bool CheckActive(int id);
        int GenealogyConflict(int id, int potentialConflict);
        void SetParent(int id, int newParent);
        void SetGeneration(int id, int generation);
    }

    [System.Serializable]
    public class Hierarchy <T>
    {
        public string name;
        public T rootObject;

        [SerializeField]
        protected int[] rootKids = new int[0];
        bool updateRootChildren = true;
        public int[] rootChildren {
            get
            {
                if (updateRootChildren)
                {
                    List<int> newRootKids = new List<int>();
                    
                    for (int i = 0; i < members.Length; i++)
                    {
                        if (genealogies[i].parentID == -1)
                        {
                            newRootKids.Add(i);
                            genealogies[i].generation = 1;
                        }
                    }
                    
                    rootKids = newRootKids.ToArray();
                    updateRootChildren = false;
                }
                return rootKids;
            }
        }

        public HierarchyStatus rootStatus;

        [SerializeField]
        protected T[] members = new T[0];
        public Genealogy[] genealogies = new Genealogy[0];
        public HierarchyStatus[] activeStatus = new HierarchyStatus[0];
        protected bool dirty = true;

        public T this[int index]
        {
            get { if (index == -10) { return rootObject; } else { return members[index]; } }
            set { dirty = true; members[index] = value; }
        }

        public int Length
        {
            get
            {
                return members.Length;
            }
        }

        int youngestGen;
        bool updateYoungestGen;
        public int YoungestGeneration {
            get {
                if (updateYoungestGen)
                {
                    youngestGen = 0;
                    for (int i = genealogies.Length - 1; i >= 0; i--)
                    {
                        if (genealogies[i].generation > youngestGen)
                        {
                            youngestGen = genealogies[i].generation;
                        }
                    }
                    updateYoungestGen = false;
                }
                return youngestGen;
            }
            
        }

        public bool GetActive (int id)
        {
            if(activeStatus[id] == HierarchyStatus.PersistentlyActive) { return true; }
            if(rootStatus == HierarchyStatus.Disabled) { return false;}

            int ancestor = genealogies[id].parentID;
            while (ancestor != -1)
            {
                if (activeStatus[ancestor] == HierarchyStatus.Disabled) { return false; }

                ancestor = genealogies[ancestor].parentID;
            }
            
            return true;
        }

        public void Add(T kid, int parentID = -1)
        {
            dirty = true;
            int newID = members.Length;
            Genealogy pg = new Genealogy();
           
            if (parentID >=0 && parentID < members.Length)
            {
                pg.parentID = parentID;
                pg.generation = genealogies[parentID].generation + 1;
                List<int> newKids = new List<int>(genealogies[parentID].childIds);
                newKids.Add(newID);
                genealogies[parentID].childIds = newKids.ToArray();
                updateYoungestGen = true;
            } else if(parentID == -1)
            {
                updateRootChildren = true;
            }

            List<T> mL = new List<T>(members);
            mL.Add(kid);
            members = mL.ToArray();
            List<Genealogy> gens = new List<Genealogy>(genealogies);
            gens.Add(pg);
            genealogies = gens.ToArray();
            List<HierarchyStatus> stats = new List<HierarchyStatus>(activeStatus);
            stats.Add(HierarchyStatus.Active);
            activeStatus = stats.ToArray();
            /*
            #if UNITY_EDITOR
            List<bool> sTargets = new List<bool>(showTargets);
            List<bool> sKids = new List<bool>(showKids);
            List<bool> sel = new List<bool>(selected);
            sTargets.Add(false); sKids.Add(false); sel.Add(false);
            showTargets = sTargets.ToArray();
            showKids = sKids.ToArray();
            selected = sel.ToArray();
            #endif 
            */
        }

        public virtual void Add(int id = -1)
        {
            
        }

        public virtual int[] Remove(int id, bool includeKids = true)
        {
            int[] newIDs = new int[Length];
            for(int i = Length-1; i >=0; i--)
            {
                newIDs[i] = i;
            }
            dirty = true;
            //1. Get this item and its genealogy. 
            updateYoungestGen = true;
            //T toRemove = members[id];
            //Genealogy tRG = genealogies[id];
            //2. Get our object list and genealogy list as lists instead of arrays.
            List<T> newList = new List<T>(members);
            List<Genealogy> newGens = new List<Genealogy>(genealogies);
            List<HierarchyStatus> stats = new List<HierarchyStatus>(activeStatus);

            /*
#if UNITY_EDITOR
            
            List<bool> sTargets = new List<bool>(showTargets);
            List<bool> sKids = new List<bool>(showKids);
            List<bool> sel = new List<bool>(selected);
#endif
            */

            if (includeKids)
            {
                //Debug.Log("Removing kids.");
                //3. Make a collection of bools: Keep or remove.
                bool[] tRem = new bool[members.Length];      //False means keep, true means remove/
                //4. Go through the genealogy of the one we're removing and ALL its kids. And all THEIR kids, etc. Mark all for removal.
                tRem[id] = true;
                //4.5. Also set the newID for any removed slots to -1.
                newIDs[id] = -1;
                int[] thoseThatGo = GetAllDescendants(id);

                for (int i = thoseThatGo.Length - 1; i >= 0; i--)
                {
                    tRem[thoseThatGo[i]] = true;
                    newIDs[thoseThatGo[i]] = -1;
                }

                //5. Go through all remaining. For each one, before this one, that has been removed, we need to lower this id by 1.
                int[] truesToPoint = new int[tRem.Length];
                int ttp = 0;
                for (int i = 0; i < tRem.Length; i++)
                {
                    truesToPoint[i] = ttp;
                    if (tRem[i]) { ttp++; }
                }
                //Debug.Log("Finally removing kids.");
                for (int i = members.Length - 1; i >= 0; i--)
                {
                    if (tRem[i])
                    {
                        
                        newList.RemoveAt(i); newGens.RemoveAt(i); stats.RemoveAt(i);

                        /*
                        #if UNITY_EDITOR
                        sTargets.RemoveAt(i); sKids.RemoveAt(i); sel.RemoveAt(i);
                        #endif
                        */
                    }
                    else
                    {
                        //Debug.Log("Keeping this one: " + i);
                        newIDs[i] -= truesToPoint[i];
                        for (int j = newGens[i].childIds.Length - 1; j >= 0; j--)
                        {
                            // I think this is wrong: newGens[i].childIds[j] -= truesToPoint[genealogies[i].childIds[j]];
                            newGens[i].childIds[j] -= truesToPoint[i];
                        }
                    }
                }
                updateRootChildren = true;
                //Debug.Log("Made it to end of include kids.");
            }
            else
            {
                //3. Move all children of the one we're axing to its parent.
                ReplaceParent(id, genealogies[id].parentID);
                List<int> newKids = new List<int>(genealogies[genealogies[id].parentID].childIds);
                newKids.Remove(id);
                genealogies[genealogies[id].parentID].childIds = newKids.ToArray();

                for (int i = members.Length - 1; i >= 0; i--)
                {
                    if(i > id) { newIDs[i]--; }
                    for (int j = newGens[i].childIds.Length - 1; j >= 0; j--)
                    {
                        if (newGens[i].childIds[j] > id)
                        {
                            newGens[i].childIds[j]--;
                        }
                    }
                }
            }
            
            members = newList.ToArray();
            genealogies = newGens.ToArray();
            activeStatus = stats.ToArray();

            /*
            #if UNITY_EDITOR
            showTargets = sTargets.ToArray();
            showKids = sKids.ToArray();
            selected = sel.ToArray();
            #endif
            */

            return newIDs;
        }

        public virtual int[] Remove(int[] ids, bool includeKids = true)
        {
            int[] newIDs = new int[Length];
            for(int i = newIDs.Length-1; i >=0; i--)
            {
                newIDs[i] = i;
            }

            bool[] removed = new bool[ids.Length];

            bool allRemoved = false;
            
            while (!allRemoved)
            {
                allRemoved = true;
                if (includeKids)
                {
                    
                    int lowest = YoungestGeneration, lID = -1;

                    for (int i = ids.Length - 1; i >= 0; i--)
                    {

                        if (!removed[i])
                        {
                            if (lowest > genealogies[ids[i]].generation)
                            {
                                lowest = genealogies[ids[i]].generation;
                                lID = i;

                            }
                            allRemoved = false;
                        }
                    }
                    // Get its kids. Remove it. Mark it removed . Mark kids as removed. 
                    int[] thoseThatGo = Remove(ids[lID]);
                    for(int i = newIDs.Length-1; i >=0; i--)
                    {
                        if(newIDs[i] < 0) { continue; }
                        newIDs[i] = thoseThatGo[newIDs[i]];
                    }
                    removed[lID] = true;
                    for (int i = thoseThatGo.Length - 1; i >= 0; i--)
                    {
                        for (int j = ids.Length - 1; j >= 0; j--)
                        {
                            if (ids[j] == thoseThatGo[i]) { removed[j] = true; }
                        }
                    }
                }
                else
                {
                    int highest = 0;
                    List<int> hIDs = new List<int>();
                    for(int i = ids.Length - 1; i >=0; i--)
                    {
                        if (!removed[i])
                        {
                            if (highest < genealogies[ids[i]].generation)
                            {
                                highest = genealogies[ids[i]].generation;
                                hIDs = new List<int>();
                                hIDs.Add(i);

                            } else if (highest == genealogies[ids[i]].generation)
                            {
                                hIDs.Add(i);
                            }
                            allRemoved = false;
                        }
                    }

                    for(int i = hIDs.Count-1; i >=0; i--)
                    {
                        int j = ids[hIDs[i]];
                        Remove(j, false);
                        removed[hIDs[i]] = true;
                    }
                }
            }
            return newIDs;
        }

        public virtual void SetAsRoot(int id)
        {
            rootObject = this[id];
            SetGeneration(id, 0);
            int[] kids = genealogies[id].childIds;
            genealogies[id].childIds = new int[0];
            Remove(id, true);
            for(int i = kids.Length-1; i >=0; i--)
            {
                if(kids[i] > id) { kids[i]--; }
                genealogies[kids[i]].parentID = -1;
                
            }
            updateRootChildren = true;
            dirty = true;
        }

        public int[] GetAllDescendants(int id)
        {
            List<Genealogy> family = new List<Genealogy>();
            List<int> childCounter = new List<int>();
            if (id >= 0)
            {
                family.Add(genealogies[id]);
                childCounter.Add(0);
            }
            else
            {
                Genealogy rootGenealogy = new Genealogy();
                rootGenealogy.childIds = rootChildren;
                family.Add(rootGenealogy);
                childCounter.Add(0);
            }
            List<int> d = new List<int>();
            Genealogy current;
            int c;
           
            while (family.Count > 0) {
                
                int fc = family.Count - 1;
                
                current = family[fc];
                c = childCounter[fc];
                if(current.childIds !=null && c < current.childIds.Length)
                {
                    childCounter[fc]++;
                    family.Add(genealogies[current.childIds[c]]);
                    childCounter.Add(0);
                    d.Add(current.childIds[c]);
                }else
                {
                    
                    family.RemoveAt(fc);
                    childCounter.RemoveAt(fc);
                    
                }
            }
            
            return d.ToArray();
        }
        
        public T[] GetInHierarchicalOrder()
        {
            int[] ho = GetAllDescendants(-1);
            T[] ordered = new T[ho.Length];
            for(int i = ho.Length-1; i >=0; i--)
            {
                ordered[i] = members[ho[i]];
            }
            return ordered;
        }

        public int GenealogyConflict(int id, int potentialConflict)
        {
                     
            if (id >= 0 && id < members.Length && potentialConflict >= 0 && potentialConflict < members.Length)
            {

                List<int> family = new List<int>();
                family.Add(id);
               
                int current;
                // Going down the heirarchy through children, make sure we never run into the potentialConflict.
                // If we ever run into it, we return the id of the child that led to this conflict.
                while (family.Count > 0)
                {
                    current = family[family.Count - 1];
                    family.RemoveAt(family.Count - 1);
                    for (int i = genealogies[current].childIds.Length - 1; i >= 0; i--)
                    {
                       if(genealogies[current].childIds[i] == potentialConflict)
                        {
                            // Return the conflicting child - the one that CONTAINS the potentialConflict
                            return current;
                        }
                        family.Add(genealogies[current].childIds[i]);
                    }
                }
                
            } else { return -2; }   // Else, if either of our numbers is wrong in some way, return -2.


            return -1;              //return -1 for no conflict.
        }
        
        public void ReplaceParent(int id, int newParent)
        {
            
            if (newParent >=0 && newParent < members.Length&& id >=0 && id < members.Length)
            {
                // make sure that the newParent is not already a descendent of id.
                int conflictID = GenealogyConflict(id, newParent);
                if (conflictID == -1)
                {
                    // No Conflict!
                    List<int> newKids = new List<int>(genealogies[newParent].childIds);

                    newKids.AddRange(genealogies[id].childIds);
                    genealogies[newParent].childIds = newKids.ToArray();
                    for(int i = genealogies[id].childIds.Length-1; i >=0; i--)
                    {
                        genealogies[genealogies[id].childIds[i]].parentID = newParent;
                        SetGeneration(genealogies[id].childIds[i], genealogies[newParent].generation + 1);
                        //genealogies[genealogies[id].childIds[i]].generation = genealogies[newParent].generation + 1;
                    }
                    genealogies[id].childIds = new int[0];
                }
                else if (conflictID >= 0)
                {
                    // newParent is a desecendent of id. ConflictID is also a descendent of id - the one that contains
                    // newParent. We want to give newParent a new, well, parent. ID's in this case. 
                    int pid = genealogies[id].parentID;
                    genealogies[newParent].parentID = pid;
                    
                    //Now we remove it from ConflictIDs child list.
                    List<int> newKids = new List<int>(genealogies[conflictID].childIds);
                    newKids.Remove(newParent);
                    genealogies[conflictID].childIds = newKids.ToArray();

                    //Next, we add it to ids parent.
                    newKids = new List<int>(genealogies[pid].childIds);
                    newKids.Add(newParent);
                    genealogies[pid].childIds = newKids.ToArray();

                    //Now we take the kids from ID and add them to newParent.
                    newKids = new List<int>(genealogies[newParent].childIds);
                    newKids.AddRange(genealogies[id].childIds);
                    genealogies[newParent].childIds = newKids.ToArray();

                    //And we change the parentID for each of them to newParent.
                    for (int i = genealogies[id].childIds.Length - 1; i >= 0; i--)
                    {
                        genealogies[genealogies[id].childIds[i]].parentID = newParent;
                        
                    }
                    // Since we had to give newParent a new generation ANYWHO, we gather all of its new children
                    // first. That way, we only need to call SetGeneration once to accomplish the same task.
                    SetGeneration(newParent, genealogies[pid].generation + 1);
                    // Then we remove all the children from id.
                    genealogies[id].childIds = new int[0];
                }
            }
            updateRootChildren = true;
        }

        public void SetParent(int id, int newParent)
        {
           
            if (id >= 0 && id < members.Length && newParent >= 0 && newParent < members.Length)
            {
                // make sure that the newParent is not already a descendent of id.
                int conflictID = GenealogyConflict(id, newParent);
                if (conflictID == -1)
                {
                    // No Conflict!
                    List<int> newKids = new List<int>(genealogies[genealogies[id].parentID].childIds);
                    newKids.Remove(id);
                    genealogies[genealogies[id].parentID].childIds = newKids.ToArray();

                    genealogies[id].parentID = newParent;
                    newKids = new List<int>(genealogies[newParent].childIds);
                    newKids.Add(id);
                    genealogies[newParent].childIds = newKids.ToArray();

                }
                else if (conflictID >= 0)
                {
                    // newParent is a desecendent of id. ConflictID is also a descendent of id - the one that contains
                    // newParent. We want to give newParent a new, well, parent. ID's in this case. 
                    // We also want to remove ID from its original parent, since it'll be in a new home soon.
                    genealogies[newParent].parentID = genealogies[id].parentID;
                    List<int> newKids = new List<int>(genealogies[genealogies[id].parentID].childIds);
                    newKids.Remove(id);
                    newKids.Add(newParent);
                    genealogies[genealogies[id].parentID].childIds = newKids.ToArray();

                    // then we want to remove newParent from conflictID's child list. 
                    newKids = new List<int>(genealogies[conflictID].childIds);
                    newKids.Remove(newParent);
                    genealogies[conflictID].childIds = newKids.ToArray();
                    // Then we want to set newParent as the newParent of ID and add ID to newParents kids.
                    genealogies[id].parentID = newParent;
                    newKids = new List<int>(genealogies[newParent].childIds);
                    newKids.Add(id);
                    genealogies[newParent].childIds = newKids.ToArray();
                }
            }
        }

        public void SetGeneration(int id, int generation)
        {
            dirty = true;
            genealogies[id].generation = generation;
            List<int> family = new List<int>();
            family.Add(id);
            while (family.Count > 0)
            {
                id = family[family.Count - 1];
                family.RemoveAt(family.Count-1);
                for (int i = 0; i < genealogies[id].childIds.Length; i++)
                {
                    genealogies[genealogies[id].childIds[i]].generation = genealogies[id].generation + 1;

                    family.Add(genealogies[id].childIds[i]);
                }
            }
            updateYoungestGen = true;
        }

        /*
        #if UNITY_EDITOR
        
        [System.NonSerialized]
        public int latestSelection;

        [System.NonSerialized]
        public bool[] selected = new bool[0], 
                      showTargets = new bool[0], 
                      showKids = new bool[0];

        [System.NonSerialized]
        public Rect drawBox;

        [System.NonSerialized]
        public float lineHeight, totalLineHeight;

        [System.NonSerialized]
        public bool rootSelected, show, showRoot, childAddedOrRemoved, isSetUp;

        [System.NonSerialized]
        public int tab = 5, holdingID = -1;

        [System.NonSerialized]
        protected int toRemove, currentIndent, mouseHolder;

        public void InspectHierarchy(SerializedProperty prop, Rect pos, GUIContent label)
        {
            CruxGUI.LoadPreferences();
            childAddedOrRemoved = false;
            EditorGUI.BeginProperty(pos, label, prop);

            if (!isSetUp)
            {
                
                if (genealogies == null)
                {
                    genealogies = new Genealogy[0];
                }
                int l = genealogies.Length;
                if(l != selected.Length)
                {
                    activeStatus = new HierarchyStatus[l];
                    selected = new bool[l];
                    showTargets = new bool[l];
                    showKids = new bool[l];
                }
                isSetUp = true;
            }

            currentIndent = 0;
            int indent = EditorGUI.indentLevel;
            EditorGUI.indentLevel = 0;
            Color b = GUI.backgroundColor;
            mouseHolder = -1;       // We start by assuming that nothing has the mouse.
            if (pos.Contains(Event.current.mousePosition)) { mouseHolder = -2; } // Inside the hierarchy. 
            GUI.Box(pos, "");
            //GUI.backgroundColor = b;
            drawBox = new Rect(pos.x, pos.y + 2, pos.width, lineHeight);
            drawBox.width *= 0.1f;

            show = EditorGUI.Toggle(drawBox, show);
            drawBox.x += drawBox.width;
            drawBox.width = pos.width * 0.9f;

            name = EditorGUI.TextField(drawBox, rootObject.GetType().Name + " Hierarchy", name);
            drawBox.x = pos.x;
            drawBox.width = pos.width;
            drawBox.y += lineHeight;
            if (!show)
            {
                totalLineHeight = lineHeight;
            }
            else
            {

                drawBox.width = pos.width*0.1f;
                showRoot = EditorGUI.Toggle(drawBox, GUIContent.none, showRoot);
                drawBox.x += drawBox.width;
                drawBox.width = pos.width * 0.5f;
                if (GUI.Button(drawBox, "Root"))
                {
                    //Check which key is down. If control is held,
                    if (!Event.current.control)
                    {
                        selected = new bool[selected.Length];
                        rootSelected = true;
                    }
                    else
                    {
                        rootSelected  = !rootSelected;
                    }
                    if (rootSelected)
                    {
                        latestSelection = -10;
                    }

                }
                drawBox.x += drawBox.width;
                drawBox.width = pos.width * 0.4f;
                rootStatus = (HierarchyStatus)EditorGUI.EnumPopup(drawBox, rootStatus);
                drawBox.y += lineHeight;
                drawBox.x = pos.x;
                drawBox.width = pos.width;
                if (showRoot)
                {
                    DrawRootObject(prop.FindPropertyRelative("rootObject"));
                }

                bool himode = EditorGUIUtility.hierarchyMode;
                EditorGUIUtility.hierarchyMode = false;
                toRemove = -1;
                DrawHierarchy(prop.FindPropertyRelative("members"), pos);
                
                if(holdingID >= 0)
                {
                    Color c = GUI.backgroundColor;
                    GUI.backgroundColor = new Color(1, 1, 1, 0.15f);
                    GUI.Box( new Rect( Event.current.mousePosition.x - pos.width * 0.5f,
                                       Event.current.mousePosition.y - lineHeight * 0.5f, 
                                       pos.width, lineHeight), 
                             new GUIContent("#" + holdingID));
                    GUI.backgroundColor = c;
                }
                if (Event.current.type == EventType.MouseDown && Event.current.button == 0)
                {
                    if (mouseHolder >= 0)
                    {
                        holdingID = mouseHolder;
                    }
                }
                else if (Event.current.type == EventType.MouseUp && Event.current.button == 0)
                {
                    if(holdingID != mouseHolder && holdingID >=0)
                    {
                        if(mouseHolder == -3)
                        {
                            SetAsRoot(holdingID);
                        }else if(mouseHolder >= 0)
                        {
                            SetParent(holdingID, mouseHolder);
                        }
                        holdingID = -1;
                    } else if (holdingID == mouseHolder)
                    {
                        holdingID = -1;
                    }
                    
                }
                
                if (toRemove >= 0)
                {
                    Remove(toRemove);
                }
                EditorGUIUtility.hierarchyMode = himode;
                totalLineHeight = drawBox.y + drawBox.height - pos.y;
                EditorGUI.indentLevel = indent;
                GUI.backgroundColor = b;
            }


            EditorGUI.EndProperty();
        }

        public virtual void DrawRootObject(SerializedProperty root)
        {
            drawBox.height = EditorGUI.GetPropertyHeight(root);
            if (drawBox.Contains(Event.current.mousePosition)) { mouseHolder = -3; }
            EditorGUI.PropertyField(drawBox, root);
            drawBox.y += drawBox.height;
            drawBox.height = lineHeight;
            if (GUI.Button(drawBox, "Add Child"))
            {
                childAddedOrRemoved = true;
                Add(-1);
                Event.current.Use();
            }
            drawBox.y += lineHeight;
        }

        public virtual void DrawHierarchy(SerializedProperty targets, Rect pos)
        {

            for (int i = 0; i < rootChildren.Length; i++)
            {
                
                List<int> family = new List<int>();
                family.Add(rootChildren[i]);

                int current;
                while (family.Count > 0)
                {
                    current = family[family.Count - 1];
                    // Drop Arrow for Show Kids 
                    if (drawBox.Contains(Event.current.mousePosition))
                    {
                        mouseHolder = current;
                    }
                    currentIndent = tab*genealogies[current].generation;
                    DrawHierarchyLine(current, pos);

                    // If Show Contents, 
                    if (showTargets[current])
                    {
                        DrawHierarchyContent(current, targets.GetArrayElementAtIndex(current), pos);
                    }

                    family.RemoveAt(family.Count - 1);
                    if (showKids[current])
                    {
                        // If show kids, wash, rinse, and repeat for each of them.
                        

                        for (int j = genealogies[current].childIds.Length - 1; j >= 0; j--)
                        {
                            family.Add(genealogies[current].childIds[j]);
                        }
                    }
                }
            }
        }
        
        public virtual void DrawHierarchyLine(int id, Rect pos)
        {
            Color c = GUI.backgroundColor;
            if (id == mouseHolder || id == holdingID)
            {
                //drawBox.x = pos.x;
                //drawBox.width = pos.width;
                
                if (mouseHolder != holdingID)
                {
                    if (id == mouseHolder) { GUI.backgroundColor = new Color(c.r * 1.1f, c.g * 1.1f, c.b * 1.1f, c.a); }
                    else { GUI.backgroundColor = new Color(c.r * 0.8f, c.g * 0.8f, c.b * 0.8f, c.a); }
                }
                else
                {
                    GUI.backgroundColor = new Color(c.r * 1.25f, c.g * 1.25f, c.b * 1.25f, c.a);
                }
                GUI.Box(drawBox, GUIContent.none);
                
            } else if (selected[id])
            {
                GUI.backgroundColor = new Color(c.r * 0.75f, c.g * 0.75f, c.b * 1.25f, c.a);
                GUI.Box(drawBox, GUIContent.none);
            }
            GUI.backgroundColor = c;
            drawBox.x = pos.x + currentIndent;
            drawBox.width = lineHeight;
            
            showKids[id] = EditorGUI.Foldout(drawBox, showKids[id], new GUIContent());
            //Selectable name box to select entries in our hierarchy.
            drawBox.x += lineHeight;
            drawBox.width = pos.width - lineHeight * 4 - currentIndent;

            // drawBox.width = GUILayoutUtility.GetRect(new GUIContent(genealogies[id].name), GUI.skin.label).width;
            if (GUI.Button(drawBox, id.ToString(), GUI.skin.label))
            {
                //Check which key is down. If control is held,
                if (!Event.current.control)
                {
                    selected = new bool[selected.Length];
                    selected[id] = true;
                    rootSelected = false;
                }else
                {
                    selected[id] = !selected[id];
                }
                if (selected[id])
                {
                    latestSelection = id;
                }
                Event.current.Use();
            }


            
            drawBox.x += drawBox.width;
            drawBox.width = lineHeight * 2;
            HierarchyStatus hs = (HierarchyStatus)EditorGUI.EnumPopup(drawBox, activeStatus[id]);
            if(hs != activeStatus[id])
            {
                activeStatus[id] = hs;
                Event.current.Use();
            }

            drawBox.x += drawBox.width;
            drawBox.width = lineHeight;
            // Check box for Show Contents.
            bool doShow = CruxGUI.CruxToggle(drawBox, showTargets[id], 7, 6);
            if(doShow != showTargets[id])
            {
                showTargets[id] = doShow;
                Event.current.Use();
            }
            drawBox.x = pos.x;
            drawBox.width = pos.width;
            drawBox.y += lineHeight;
        }

        public virtual void DrawHierarchyContent(int id, SerializedProperty targ, Rect pos)
        {
            //  -Edit entry name / Delete Entry button.
            if (drawBox.Contains(Event.current.mousePosition))
            {
                mouseHolder = id;
            }
            drawBox.width -= lineHeight;
            //drawBox.width *= 0.5f;
            //genealogies[id].name = EditorGUI.TextField(drawBox, genealogies[id].name);
            //drawBox.x += drawBox.width;
            if (GUI.Button(drawBox, "Add Child"))
            {
                childAddedOrRemoved = true;
                Add(id);
            }
            drawBox.x += drawBox.width;
            drawBox.width = lineHeight;
            
            if (CruxGUI.RemoveButton(drawBox))
            {
                childAddedOrRemoved = true;
                toRemove = id;
            }

            drawBox.width = pos.width;
            drawBox.x = pos.x;
            drawBox.y += lineHeight;
            
            //  -draw contents

            drawBox.height = EditorGUI.GetPropertyHeight(targ);
            if (drawBox.Contains(Event.current.mousePosition))
            {
                mouseHolder = id;
            }
            EditorGUI.PropertyField(drawBox, targ);

            //  -move down a line
            drawBox.y += drawBox.height;
            drawBox.height = lineHeight;
            //  -reset indent.
            //EditorGUI.indentLevel = currentIndent;
        }

        #endif
        */

    }

    [System.Serializable]
    public class Genealogy
    {
        //public string name;
        public int parentID = -1, generation = 1;
        
        public int[] childIds = new int[0];
        public Genealogy()
        {
            //name = n;
            parentID = -1;
            generation = 1;
            childIds = new int[0];
        }

        public Genealogy(Genealogy original)
        {
            //name = original.name;
            parentID = original.parentID;
            generation = original.generation;
            childIds = original.childIds;
            
        }
    }

    
}