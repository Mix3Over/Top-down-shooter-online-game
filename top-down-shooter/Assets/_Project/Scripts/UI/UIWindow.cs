using UnityEngine;

public class UIWindow : MonoBehaviour
{
    private GameObject _content;

    protected virtual void Awake()
    {
        _content = transform.GetChild(0).gameObject;
        Close();
    }

    public virtual void Open()
    {
        _content.SetActive(true);
    }

    public virtual void Close()
    {
        _content.SetActive(false);
    }
}
