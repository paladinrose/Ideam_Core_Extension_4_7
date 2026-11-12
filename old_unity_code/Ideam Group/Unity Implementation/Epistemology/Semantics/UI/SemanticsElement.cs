using System;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;
using UnityEngine.UIElements;

using Ideam.Epistemology.Semantics;
using Ideam.Science.Formal.Linguistics;
using Ideam.Unity;
namespace Ideam.Unity.UI
{
    public class SemanticsElement : DropList
    {
        #region UXML Factory
        
        public new class UxmlFactory : UxmlFactory<SemanticsElement, UxmlTraits> { }

        public new class UxmlTraits : DropList.UxmlTraits
        {
           
            public override IEnumerable<UxmlChildElementDescription> uxmlChildElementsDescription
            {
                get { yield break; }
            }

            public override void Init(VisualElement ve, IUxmlAttributes bag, CreationContext cc)
            {
                base.Init(ve, bag, cc);
                var semanticsElement = ve as SemanticsElement;

            }
        }

        #endregion

        protected override event Action<int> onAdd;
        public override event Action<int> OnAdd
        {
            add { onAdd += value; if (onAdd.GetInvocationList().Length > 0) { add.style.display = DisplayStyle.Flex; } else { add.style.display = DisplayStyle.None; } }
            remove { onAdd -= value; if (onAdd.GetInvocationList().Length > 0) { add.style.display = DisplayStyle.Flex; } else { add.style.display = DisplayStyle.None; } }

        }
        protected UnityLanguageAsset languageAsset;

        protected ISemantic semanticObject;
        protected int[] sememeIDs;

        protected bool toggleLanguageAssetElement;
        protected VisualElement languageAssetElement;
        protected DropList allEntriesList;

        protected bool allowAddingSemantics;
        public bool AllowAddingSemantics
        {
            get { return allowAddingSemantics; }
            set
            {
                if (value != allowAddingSemantics)
                {
                    allowAddingSemantics = value;
                    if (allowAddingSemantics)
                    {
                        OnAdd += AddSemantic;
                    }
                    else
                    {
                        OnAdd -= AddSemantic;
                    }
                }
            }
        }

        protected bool allowRemovingSemantics;
        public bool AllowRemovingSemantics
        {
            get { return allowRemovingSemantics; }
            set
            {
                if (value != allowRemovingSemantics)
                {
                    allowRemovingSemantics = value;
                    if (allowRemovingSemantics)
                    {
                        ShowRemoveButtons = true;
                        OnRemove += RemoveSemantic;
                    }
                    else
                    {
                        ShowRemoveButtons = false;
                        OnRemove -= RemoveSemantic;
                    }
                }
            }
        }

        public SemanticsElement() : base()
        {
            
            ShowSelection = false;
            ShowSearch = true;
            ShowNoneOption = false;
            ShowRemoveButtons = false;

        }

        public SemanticsElement(ISemantic obj) : base()
        {
            semanticObject = obj;

            BuildSemanticsList();

            ShowSelection = false;
            ShowSearch = true;
            ShowNoneOption = false;
            ShowRemoveButtons = false;

        }

        public SemanticsElement(string label): base(label)
        {
            ShowSelection = false;
            ShowSearch = true;
            ShowNoneOption = false;
            ShowRemoveButtons = false;

        }

        public SemanticsElement(ISemantic obj, string label) : base(label)
        {
            semanticObject = obj;


            BuildSemanticsList();

            ShowSelection = false;
            ShowSearch = true;
            ShowNoneOption = false;
            //ShowRemoveButtons = true;

            //OnAdd += AddSemantic;
            //OnRemove += RemoveSemantic;
        }

        protected virtual void BuildSemanticsList()
        {
            
            if (semanticObject is ILanguageProvider languageProvider)
            {
                languageAsset = languageProvider.GetLanguage<UnityLanguageAsset>();
            }
            if (!languageAsset)
            {
                languageAsset = UnityLanguageAsset.CurrentLanguageAsset;
            }

            if (!languageAsset) 
            {
                Add(new Label("No Language Asset could be located."));
                return; 
            }
            
            sememeIDs = semanticObject.GetSememeIDs();
            string[] sememeLabels = languageAsset.GetFormatted(sememeIDs);

            BuildList(sememeLabels); //, sememeIDs);
            
        }
        
        public override void AddEntry()
        {
            if (toggleLanguageAssetElement) { toggleLanguageAssetElement = false; rootParent.Remove(languageAssetElement); return; }

            toggleLanguageAssetElement = true;
            languageAssetElement = new VisualElement();
            languageAssetElement.AddToClassList("background");
            languageAssetElement.style.position = Position.Absolute;
            languageAssetElement.style.width = languageAssetElement.style.height = new StyleLength(StyleKeyword.Auto);
            
            FindParents();
            rootParent.Add(languageAssetElement);

            Vector2 minSize = new Vector2(rootParent.layout.width * 0.5f, rootParent.layout.height * 0.5f);
            Vector2 maxSize = new Vector2(rootParent.layout.width * 0.85f, rootParent.layout.height * 0.75f);

            languageAssetElement.style.minWidth = new StyleLength(minSize.x);
            languageAssetElement.style.minHeight = new StyleLength(minSize.y);
            languageAssetElement.style.maxWidth = new StyleLength(maxSize.x);
            languageAssetElement.style.maxHeight = new StyleLength(maxSize.y);
            
            if (languageAsset == null) 
            {
                Label noAsset = new Label("No language asset found. You must assign a language asset to add entries.");
                languageAssetElement.Add(noAsset);

                Button closeButton = new Button(() => { rootParent.Remove(languageAssetElement); });
                closeButton.text = "Ok";
                languageAssetElement.Add(closeButton);
                return;
            }

            Label languageAssetLabel = new Label("Add from " + languageAsset.name);
            languageAssetElement.Add(languageAssetLabel);

            allEntriesList = new DropList("");
            languageAssetElement.Add(allEntriesList);

            allEntriesList.ShowDropLock = false;
            allEntriesList.Docked = true;
            allEntriesList.AllowMultiSelect = true;
            allEntriesList.OnAdd += addID => 
            {
                if (!string.IsNullOrWhiteSpace(allEntriesList.SearchFieldText)) 
                {
                    string newSemanticLabel = languageAsset.ValidateGrammar(allEntriesList.SearchFieldText);

                    if (newSemanticLabel == languageAsset.Invalid)
                    {
                        allEntriesList.SearchFieldText = newSemanticLabel;
                        return;
                    }
                   
                    int sememe = languageAsset.GetSemanticID(allEntriesList.SearchFieldText);
                    //semanticObject.AddSememe(sememe);

                    int l = listElements.Length;
                    ListElement newEntry = new ListElement(newSemanticLabel);
                    newEntry.listValue = l;
                    newEntry.clicked += () => { Select(l, allowMultiSelect); };
                    newEntry.OnRemove += RemoveEntry;
                    if (showRemoveButtons) { newEntry.HasRemove = true; }
                    System.Array.Resize(ref listElements, l + 1);
                    System.Array.Resize(ref sememeIDs, l + 1);
                    sememeIDs[l] = sememe;
                    listElements[l] = newEntry;
                    listScroll.Add(newEntry);

                    onAdd?.Invoke(l);
                    rootParent.Remove(languageAssetElement);
                }
            };

            string[] allEntries = languageAsset.GetAllEntries();

            allEntriesList.BuildList(allEntries);

            List<int> lexHeaders = new List<int>();
            for(int i =0; i < allEntries.Length; i++)
            {
                if (allEntries[i].StartsWith("<Lexicon>")) lexHeaders.Add(i);
            }
            allEntriesList.DisableEntries(lexHeaders.ToArray());

            VisualElement buttonBar = new VisualElement();
            buttonBar.style.flexDirection = FlexDirection.Row;
            languageAssetElement.Add(buttonBar);

            Button okButton = new Button() { text = "Add Selection" };
            okButton.clicked += () => 
            {
                int[] selectedEntries = allEntriesList.FullSelection;
                List<ListElement> newElementsList = new List<ListElement>(listElements);
                List<int> onAddsList = new List<int>();
                for(int i =0; i < selectedEntries.Length; i++)
                {
                    
                    int entryID = selectedEntries[i];
                    string entry = allEntriesList.listElements[entryID].Label;

                    if (string.IsNullOrEmpty(entry)) { continue; }

                    if (!allowRepeatEntries)
                    {
                        for (int j = 0; j < listElements.Length; j++)
                        {
                            if (listElements[j].Label == entry) { continue; }
                        }
                    }
                    int l = listElements.Length;

                    ListElement newEntry = new ListElement(entry);
                    newEntry.listValue = l;
                    newEntry.clicked += () => { Select(l, allowMultiSelect); };
                    newEntry.OnRemove += RemoveEntry;
                    if (showRemoveButtons) { newEntry.HasRemove = true; }

                    newElementsList.Add(newEntry);
                    listScroll.Add(newEntry);

                    onAddsList.Add(l);

                }

                listElements = newElementsList.ToArray();
                rootParent.Remove(languageAssetElement);

                if (onAdd != null)
                {
                    for (int i = 0; i < onAddsList.Count; i++)
                    {
                        onAdd.Invoke(onAddsList[i]);
                    }
                }
            };
            buttonBar.Add(okButton);

            Button cancelButton = new Button() { text = "Cancel" };
            cancelButton.clicked += () => { rootParent.Remove(languageAssetElement); };
            buttonBar.Add(cancelButton);
            
        }

        protected virtual void AddSemantic(int id)
        {
            //1. Get the string from the labels in our list element at 'id'
            string newSemanticLabel = listElements[id].Label;
            
            // - Filter newSemanticLabel for any unuseable labels. Remove the new listElement if it falls in this camp.
            
            newSemanticLabel = languageAsset.ValidateGrammar(newSemanticLabel);

            if(newSemanticLabel == languageAsset.Invalid)
            {
                //Remove he new list element. It's not a valid piece of grammar as far as this language asset is concerned.
                RemoveEntry(id);
                searchField.SetValueWithoutNotify(string.Empty);
                return;
            }

            //2. Send the string away to the language asset to get the id of the related sememe.

            //3. The language asset takes the string and finds lexemes for it. It then queries the semantics manager for a sememe id that represents that combo of lexemes.
            //4. The semantics manager uses space math wizardry to take in a list of lexemes, get their individual semes and sememes, and use them to identify
            //   a sememe that already fits this bill, or create a new sememe to fit this specific context. It returns the ID of this sememe to the language asset.
            //5. The language asset recieves the id from the semantics manager and forwards it back to this function. We add that id to the semantic object in question.
            int newSememeID = languageAsset.GetSemanticID(newSemanticLabel);

            semanticObject.AddSememe(newSememeID);
        }

        protected virtual void RemoveSemantic(int id)
        {
            semanticObject.RemoveSememe(id);
        }
    }
}