using System.Collections;
using System.Collections.Generic;

using UnityEngine;
using UnityEngine.UIElements;

using UnityEditor;
using UnityEditor.UIElements;

using Ideam.Unity;


[CustomPropertyDrawer(typeof(UnityGamePieceProperty))]
public class UnityGamePiecePropertyDrawer : PropertyDrawer
{
    public override VisualElement CreatePropertyGUI(SerializedProperty property)
    {
        return base.CreatePropertyGUI(property);

        VisualElement propertyElement = new VisualElement();
        propertyElement.style.flexDirection = FlexDirection.Row;

        
        var amountField = new PropertyField(property.FindPropertyRelative("amount"));

        return propertyElement;
    }
}
