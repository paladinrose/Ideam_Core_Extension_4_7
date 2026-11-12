
namespace Ideam.Science.Formal.Linguistics
{
    public interface ILanguageProvider 
    {
        T GetLanguage<T>() where T : ILanguage;

        
    }
}
