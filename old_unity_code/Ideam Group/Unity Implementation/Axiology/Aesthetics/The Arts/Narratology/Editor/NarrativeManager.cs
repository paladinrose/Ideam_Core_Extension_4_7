using System;
using System.IO;
using System.Text.RegularExpressions;
using System.Linq;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;
using UnityEngine.UIElements;
using UnityEditor;

using Ideam.Unity.UI;
using Ideam.Unity.Axiology.Aesthetics.The_Arts.Narratology;


namespace Ideam.Unity
{
    
    public class NarrativeManager : EditorWindow
    {
        [MenuItem("Assets/Create/Ideam/Narratology/Create Narrative Directories")]
        public static void CreateNarrativeFolders()
        {
            string folderPath = AssetDatabase.GetAssetPath(Selection.activeObject);

            AssetDatabase.CreateFolder(folderPath, "Characters");
            AssetDatabase.CreateFolder(folderPath, "Props");
            AssetDatabase.CreateFolder(folderPath, "Locations");
            AssetDatabase.CreateFolder(folderPath, "Incidents");
            AssetDatabase.CreateFolder(folderPath, "Plots");

         }

        [MenuItem("Tools/Ideam/Narrative Manager")]
        public static void OpenNarrativeManager()
        {
             narrativeManager = GetWindow<NarrativeManager>();

            narrativeManager.Show();
        }

        static NarrativeManager narrativeManager;

        static UnityNarrative narrativeAsset;

        VisualTreeAsset uiAsset,
            textGeneratorAsset,
            characterGraphAsset;

        VisualElement root;
        TabBar narrativeTextsTabs;

        private void OnEnable()
        {
            rootVisualElement.AddToClassList("ideamRoot");
            BuildBaseUI();
        }

        private void OnDestroy()
        {
            
        }

        public UnityNarrative NarrativeAsset 
        {   get { return narrativeAsset; } 
            set { if(value != narrativeAsset) SetNarrativeAsset(value); } }

        void SetNarrativeAsset(UnityNarrative newAsset)
        {
            if (narrativeAsset)
            {

            }

            narrativeAsset = newAsset;
            BuildNarrativeUI();
        }

        void BuildBaseUI()
        {
            rootVisualElement.Clear();
            ObjectSelectorField assetField = new ObjectSelectorField("Narrative Asset");
            rootVisualElement.Add(assetField);
            
            assetField.Initialize<UnityNarrative>(SetNarrativeAsset);
          
            assetField.Select(narrativeAsset);
            
            //VisualElement root = uiAsset.CloneTree();
            root = new VisualElement();
            rootVisualElement.Add(root);

        }

        void BuildNarrativeUI()
        {
            root.Clear();

            //TabBar rootTabs = root.Q<TabBar>();
            TabBar rootTabs = new TabBar();
            root.Add(rootTabs);

            rootTabs.AddPage(TextGeneratorTool, "Text Generator");

        }

        VisualElement TextGeneratorTool()
        {
            //VisualElement textGenerator = textGeneratorAsset.CloneTree();
            VisualElement textGenerator = new VisualElement();
            
            narrativeTextsTabs = new TabBar();
            textGenerator.Add(narrativeTextsTabs);

            //textEntryField = textGenerator.Q<TextField>("TextEntryField");
            for(int i =0; i < narrativeAsset.narrativeTexts.Count; i++)
            {
                int id = i;
                narrativeTextsTabs.AddPage(() => { return NarrativeTextPage(id); }, narrativeAsset.narrativeTexts[i].title);
            }
            narrativeTextsTabs.AddPage(AddNewNarrativeText, "New");
            return textGenerator;
        }

        VisualElement AddNewNarrativeText()
        {
            int id = narrativeAsset.narrativeTexts.Count;
            narrativeAsset.narrativeTexts.Add(new NarrativeText());

            return NarrativeTextPage(id);
        }
        VisualElement NarrativeTextPage(int textID)
        {
            NarrativeText nT = narrativeAsset.narrativeTexts[textID];
            VisualElement narrativeText = new VisualElement();

            TextField titleField = new TextField("Title") { value = nT.title };
            narrativeText.Add(titleField);

            VisualElement splitElement = new VisualElement();
            splitElement.style.flexDirection = FlexDirection.Row;
            splitElement.style.flexGrow = 1f;
            narrativeText.Add(splitElement);

            TextField rawTextField = new TextField("")
            {
                value = nT.rawText
            };
            rawTextField.style.flexWrap = Wrap.Wrap;
            rawTextField.RegisterValueChangedCallback(evt => { nT.rawText = evt.newValue; });
            splitElement.Add(rawTextField);

            TextField parsedTextField = new TextField("")
            {
                value = nT.ParsedText
            };
            parsedTextField.isReadOnly = true;
            splitElement.Add(parsedTextField);


            return narrativeText;
        }


    }
}