using System;
using UnityEngine.UIElements;
using Ideam.Science.Formal.Mathematics.Geometry.Splines;


namespace Ideam.Unity.Science.Formal.Mathematics
{
    public abstract class SplineBaseUI<T> : VisualElement
    where T:SplineBase
    {
        public virtual event Action<int> OnSelectPoint, OnSelectSection;
        public virtual event Action<int, CurveType> OnSectionCurveChange;
        public virtual event Action<int, float> OnSectionWeightChange;

        protected T spline;
        protected TextField nameField;
        protected Foldout defaultsFoldout;
        protected EnumField defaultCurveField;
        protected FloatField defaultWeightField, defaultDropValueField;
        protected Label totalWeightField, totalLengthField;

        protected SliderInt pointSelector, sectionSelector;
        protected VisualElement pointElement, sectionElement;

        protected int selectedPoint = -1, selectedSection = -1;
        public SplineBaseUI(T spline) : base()
        {
            this.spline = spline;

            nameField = new TextField("Name") { value = spline.name, isDelayed = true };
            Add(nameField);
            nameField.RegisterValueChangedCallback(evt => { spline.name = evt.newValue; });

            defaultsFoldout = new Foldout() { text = "Default Point Settings", value = false };
            Add(defaultsFoldout);

            defaultCurveField = new EnumField("Default Curve Type", spline.defaultCurveType);
            defaultCurveField.RegisterValueChangedCallback(evt => { spline.defaultCurveType = (CurveType)evt.newValue; });
            defaultsFoldout.Add(defaultCurveField);

            defaultWeightField = new FloatField("Default Weight");
            defaultWeightField.value = spline.defaultWeight;
            defaultWeightField.isDelayed = true;
            defaultWeightField.RegisterValueChangedCallback(evt => { spline.defaultWeight = evt.newValue; });
            defaultsFoldout.Add(defaultWeightField);

            defaultDropValueField = new FloatField("Default DropValue");
            defaultDropValueField.value = spline.defaultDropValue;
            defaultDropValueField.isDelayed = true;
            defaultDropValueField.RegisterValueChangedCallback(evt => { spline.defaultDropValue = evt.newValue; });
            defaultsFoldout.Add(defaultDropValueField);

            totalWeightField = new Label("Total Weight : " + spline.GetTotalWeight());
            Add(totalWeightField);

            totalLengthField = new Label("Total Length : " + spline.GetTotalSplineLength(0.1f));
            Add(totalLengthField);

            pointSelector = new SliderInt("Selected Point", -1, spline.Length-1) { showInputField = true, value = -1 };
            pointSelector.RegisterValueChangedCallback(SelectPoint);
            Add(pointSelector);
            pointElement = new VisualElement();
            Add(pointElement);

            sectionSelector = new SliderInt("Selected Section", -1, spline.Sections-1) { showInputField = true, value = -1 };
            sectionSelector.RegisterValueChangedCallback(SelectSection);
            Add(sectionSelector);
            sectionElement = new VisualElement();
            Add(sectionElement);

        }

        protected virtual void SelectSection(ChangeEvent<int> evt)
        {
            if(evt.newValue == selectedSection) { return; }

            selectedSection = evt.newValue;
            sectionElement.Clear();
           
            if(selectedSection == -1)
            {
                sectionElement.style.display = DisplayStyle.None;
                
                OnSelectSection?.Invoke(selectedSection);

                return;
            }

            sectionElement.style.display = DisplayStyle.Flex;

            EnumField selectedSectionCurveField = new EnumField("Curve", spline.sections[selectedSection].curveType);
            selectedSectionCurveField.RegisterValueChangedCallback( evt => 
            {
                SplineSection sec = spline.sections[selectedSection];
                sec.curveType = (CurveType)evt.newValue;
                spline.sections[selectedSection] = sec;
                spline.splineDirty = spline.anchorsDirty = true;

                OnSectionCurveChange?.Invoke(selectedSection, sec.curveType);
            });
            sectionElement.Add(selectedSectionCurveField);

            FloatField selectedSectionWeightField = new FloatField("Weight");
            selectedSectionWeightField.value = spline.sections[selectedSection].weight;
            selectedSectionWeightField.isDelayed = true;
            selectedSectionWeightField.RegisterValueChangedCallback( evt => 
            {
                SplineSection sec = spline.sections[selectedSection];
                sec.weight = evt.newValue;
                spline.sections[selectedSection] = sec;
                spline.anchorsDirty = spline.splineDirty = true;

                OnSectionWeightChange?.Invoke(selectedSection, sec.weight);
            });
            sectionElement.Add(selectedSectionWeightField);

            FloatField selectedSectionLengthField = new FloatField("Length");
            selectedSectionLengthField.value = spline.sections[selectedSection].length;
            selectedSectionLengthField.isReadOnly = true;
            /*
            selectedSectionLengthField.isDelayed = true;
            selectedSectionLengthField.RegisterValueChangedCallback(evt =>
            {
                SplineSection sec = spline.sections[selectedSection];
                sec.length = selectedSectionLengthField.value;
                spline.sections[selectedSection] = sec;
            });
            */
            sectionElement.Add(selectedSectionLengthField);

            OnSelectSection?.Invoke(selectedSection);
        }

        protected virtual void SelectPoint(ChangeEvent<int> evt) 
        {
            SelectPoint(evt.newValue);
        }
        public abstract void SelectPoint(int newPointID);
       
    }
}