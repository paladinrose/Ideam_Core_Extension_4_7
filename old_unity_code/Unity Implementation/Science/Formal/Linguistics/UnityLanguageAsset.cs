using System;
using System.Text;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Epistemology.Semantics;
using Ideam.Science.Formal.Linguistics;
using Ideam.Science.Formal.Linguistics.Grammar;
using Ideam.Science.Formal.Linguistics.Lexicology;

using Ideam.Unity.Semantics;

namespace Ideam.Unity
{
    [CreateAssetMenu(menuName ="Ideam/Linguistics/Language Asset")]
    public class UnityLanguageAsset : ScriptableObject, ILanguage
    {
        
        public static UnityLanguageAsset CurrentLanguageAsset;

        public UnitySemanticManager semanticManager;

        public UnityGrammar[] grammars = new UnityGrammar[1];
        public UnityLanguageLect[] lects = new UnityLanguageLect[1];
        public UnityLexicon[] lexicons = new UnityLexicon[1];

        [SerializeField]
        protected string invalid = "~INVALID~";
        public string Invalid { get { return invalid; } }

        public T GetGrammars<T>() where T : IGrammar
        {
            throw new System.NotImplementedException();
        }

        public T GetLects<T>() where T : ILect
        {
            throw new System.NotImplementedException();
        }

        public T GetLexicons<T>() where T : ILexicon
        {
            throw new System.NotImplementedException();
        }

        public T GetPhrase<T>(int[] sememes) where T : IPhrase
        {
            throw new System.NotImplementedException();
        }

        bool updateAllEntries = true;
        string[] allEntries = new string[0];
        public string[] GetAllEntries()
        {
            if(allEntries == null || allEntries.Length == 0 || updateAllEntries == true)
            {
                List<string> newAllEntries = new List<string>();
                for(int l = 0; l < lexicons.Length; l++)
                {
                    UnityLexicon lexicon = lexicons[l];
                    newAllEntries.Add("<Lexicon>" + lexicon.name);
                    newAllEntries.AddRange(lexicon.ToString(new int[0]));
                }

                allEntries = newAllEntries.ToArray();
            }

            return allEntries;
        }

        public string[] GetFormatted(int[] sememes)
        {
            if(sememes.Length == 0) { return new string[0]; }
            string[] formattedSememes = lexicons[0].ToString(sememes);
            
            return formattedSememes;
        }
        
        public T GetSentence<T>(int[] sememes) where T : ISentence
        {
            throw new System.NotImplementedException();
        }
        
        public string[] GetFormattedSentence(UnitySentence sentence)
        {
            throw new NotImplementedException();
        }

        public int GetSemanticID(string label)
        {
            for(int i = 0; i < lexicons.Length; i++)
            {
                UnityLexicon l = lexicons[i];
                for (int j = 0; j < l.lexemes.Length; j++) 
                {
                    if (l.lexemes[j].MorphologicalMatch(label))
                    {
                        return l.lexemes[j].sememe;
                    }
                }
            }

            updateAllEntries = true;
            
            UnityLexicon toAddTo = lexicons[0];
            
            int lexemeID = toAddTo.GetLexeme(label);

            UnityLexeme lexemeToAdd;
            
            if (lexemeID >= 0)
            {
                lexemeToAdd = toAddTo.lexemes[lexemeID];

                return lexemeToAdd.sememe;
            }
            else
            {

                //Break down the label using spaces, hyphons, and then syllabic breakdowns.
                lexemeToAdd = new UnityLexeme();

                string unidentifiedComponents = TrimKnownSemes(label, 0, out List<int> knownSemes);

                //Now we get NEW  semes for our unidentifiedComponents. Well, one for now...
                int newSeme = NewSemeMorphology(unidentifiedComponents, 0);
                knownSemes.Add(newSeme);

                UnityMorpheme morphemeToAdd = new UnityMorpheme() { form = label, formSemes = knownSemes.ToArray() };
                lexemeToAdd.morphemes = new UnityMorpheme[] { morphemeToAdd };

                //Go through my lexemes and match those parts (lexeme.MorphologicalMatch).
                //Collect the Semes and Forms from each matching lexeme into a new sememe and a new lexeme.
                

                Sememe newSememe = new Sememe();
                newSememe.semes = morphemeToAdd.formSemes;

                //Register the new sememe with the semantics manager (semanticsManager.GetSememeID(newSememe))
                int newSememeID = semanticManager.GetSememeID(newSememe);
                //Set the Sememe ID on the new lexeme.
                lexemeToAdd.sememe = newSememeID;
                //Add the new lexeme to the lexicon toAddTo.
                List<UnityLexeme> newLexes = new List<UnityLexeme>(toAddTo.lexemes);
                newLexes.Add(lexemeToAdd);
                toAddTo.lexemes = newLexes.ToArray();

                return newSememeID;
            }
        }

        private int NewSemeMorphology(string unidentifiedComponents, int lexiconID = 0)
        {
            UnityLexicon lexicon = lexicons[lexiconID];
            int newSeme = semanticManager.NewSeme();
            List<string> semeMorphs = new List<string>(lexicon.semeMorphology);
            List<int> semeIDs = new List<int>(lexicon.semeMorphologyIDs);
            semeMorphs.Add(unidentifiedComponents);
            semeIDs.Add(newSeme);
            lexicon.semeMorphology = semeMorphs.ToArray();
            lexicon.semeMorphologyIDs = semeIDs.ToArray();
            return newSeme;
        }

        private string TrimKnownSemes(string label, int lexiconID, out List<int> knownSemes)
        {
            string trimmed = label;

            UnityLexicon lexicon = lexicons[lexiconID];
            knownSemes = new List<int>();
            for(int i =0; i < lexicon.lexemes.Length; i++)
            {

            }

            return trimmed;
        }

        public virtual string ValidateGrammar(string newSemanticLabel)
        {
            //SPACE MAGIC!!!
            return newSemanticLabel;

            //return invalid;
        }
    }

    [Serializable]
    public class UnityGrammar : IGrammar
    {
        public string name;
        public T GetPhrase<T>(int[] sememes) where T : IPhrase
        {
            throw new NotImplementedException();
        }

        public T GetSentence<T>(int[] sememes) where T : ISentence
        {
            throw new NotImplementedException();
        }

        public T[] GetSyntaxes<T>() where T : ISyntax
        {
            throw new NotImplementedException();
        }
    }

    [Serializable]
    public class UnityLanguageLect : ILect
    {
        public string name;
    }

    [Serializable]
    public class UnityLexicon : ILexicon
    {
        public string name;
        public int[] semeMorphologyIDs = new int[0];
        public string[] semeMorphology = new string[0];

        public UnityLexeme[] lexemes = new UnityLexeme[0];

        
        public T[] Lexemes<T>(int[] lexemeIDs) where T : ILexeme
        {
            T[] tLexes = new T[lexemeIDs.Length];
            for(int i =0; i < lexemeIDs.Length; i++)
            {

            }
            return tLexes;
        }


        public string[] ToString(int[] lexemeIDs)
        {
            if(lexemeIDs == null || lexemeIDs.Length == 0)
            {
                lexemeIDs = new int[lexemes.Length];
                for(int i =0; i < lexemes.Length; i++)
                {
                    lexemeIDs[i] = i;
                }
            }

            string[] lexStrings = new string[lexemeIDs.Length];
            for(int i =0; i < lexemeIDs.Length; i++)
            {
                lexStrings[i] = ToString(lexemeIDs[i]);
            }
            return lexStrings;
        }

        public string ToString(int lexemeID)
        {
            UnityLexeme lexeme = lexemes[lexemeID];
            return lexeme.ToString();
            
        }

        public int GetLexeme(string label)
        {
            for(int i =0; i < lexemes.Length; i++)
            {
                if (lexemes[i].MorphologicalMatch(label)) { return i; }
            }
            
            return -1;
        }
    }

    [Serializable]
    public class UnitySentence : ISentence
    {
        public UnityPhrase[] phrases = new UnityPhrase[0];
        public T GetPhrase<T>(int phraseID) where T : IPhrase
        {
            throw new NotImplementedException();
        }

        public int GetPhraseID<T>(T phrase) where T : IPhrase
        {
            throw new NotImplementedException();
        }

        public T[] GetPhrases<T>() where T : IPhrase
        {
            throw new NotImplementedException();
        }
    }

    [Serializable]
    public class UnityPhrase : IPhrase
    {
        int[] lexemes = new int[0];
        int[] inflections = new int[0];

        public int GetInflection(int id)
        {
            if (id < 0 || id >= inflections.Length) { return -1; }

            return inflections[id];
        }

        public int GetLexeme(int id)
        {
            if(id <0 || id >= lexemes.Length) { return -1; }

            return lexemes[id];
        }

        public void GetLexemes(out int[] lexemeIDs, out int[] inflections)
        {
            lexemeIDs = lexemes; inflections = this.inflections;
        }
    }

    [Serializable]
    public class UnityLexeme : ILexeme
    {
        public int sememe;
        public UnityMorpheme[] morphemes = new UnityMorpheme[0];
        public UnityPhoneme[] phonemes = new UnityPhoneme[0];
        
        public bool MorphologicalMatch(string toMatch)
        {
            if(morphemes.Length == 0) { return false; }

            StringBuilder matching = new StringBuilder();
            matching.Append(morphemes[0].form);

            if(matching.ToString() == toMatch) { return true; }
            for(int i =1; i < morphemes.Length; i++)
            {
                matching.Append(morphemes[i].form);
                if(matching.ToString() == toMatch) { return true; }
            }
            return false;
        }
        public override string ToString()
        {
            return morphemes[0].form;
        }
    }

    [Serializable]
    public class UnityMorpheme : IMorpheme
    {
        public string form;
        public int[] formSemes = new int[0];
    }

    [Serializable]
    public class UnityPhoneme : IPhoneme
    {
        public AudioClip form;
    }
}