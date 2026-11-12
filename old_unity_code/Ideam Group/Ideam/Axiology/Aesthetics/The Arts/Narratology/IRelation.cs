namespace Ideam.Axiology.Aesthetics.The_Arts.Narratology
{
    public interface IRelation<T> where T:INarreme
    {
        T Subject { get; }
        Tn GetSubject<Tn>() where Tn : INarreme;
    }

}
