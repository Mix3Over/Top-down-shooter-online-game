using Cysharp.Threading.Tasks;
using UnityEngine;
using UnityEngine.SceneManagement;

public class SceneLoader : MonoBehaviour
{
    public static bool IsUILoaded;

    private const string GameSceneName = "GameScene";
    private const string UISceneName = "UIScene";
    private const string AudioSceneName = "AudioScene";

    private static bool _isGameLoadingOrLoaded;

#if UNITY_EDITOR
    [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterAssembliesLoaded)]
    private static void ResetStatics()
    {
        _isGameLoadingOrLoaded = false;
    }
#endif

#if !UNITY_EDITOR
    private void Awake()
    {
        LoadScenes().Forget();
    }
#else
    private void Start()
    {
        LoadScenesAsync().Forget();
    }
#endif

    private static async UniTask LoadScenesAsync()
    {
#if UNITY_EDITOR
        if (SceneManager.GetSceneByName(GameSceneName).isLoaded)
            await SceneManager.UnloadSceneAsync(GameSceneName);

        if (!SceneManager.GetSceneByName(UISceneName).isLoaded)
            await SceneManager.LoadSceneAsync(UISceneName, LoadSceneMode.Additive);
        IsUILoaded = true;

        if(!SceneManager.GetSceneByName(AudioSceneName).isLoaded)
            await SceneManager.LoadSceneAsync(AudioSceneName, LoadSceneMode.Additive);

        await OpenMainMenuAsync();
#else
        await SceneManager.LoadSceneAsync(UISceneName, LoadSceneMode.Additive);
        await SceneManager.LoadSceneAsync(AudioSceneName, LoadSceneMode.Additive);
        IsUILoaded = true;
        OpenMainMenu();
#endif
    }

    public static async UniTask StartGameAsync()
    {
        if (_isGameLoadingOrLoaded)
        {
            Debug.LogError("GameScene is already loaded");
            return;
        }

        _isGameLoadingOrLoaded = true;

        await SceneManager.LoadSceneAsync(GameSceneName, LoadSceneMode.Additive);
        SceneManager.SetActiveScene(SceneManager.GetSceneByName(GameSceneName));

        UIManager.Instance.GameplayUI.Open();
        UIManager.Instance.MainMenuUI.Close();
    }

    public static async UniTask OpenMainMenuAsync()
    {
        if (_isGameLoadingOrLoaded)
        {
            await UniTask.WaitUntil(() => SceneManager.GetSceneByName(GameSceneName).isLoaded);
            await SceneManager.UnloadSceneAsync(GameSceneName);
            _isGameLoadingOrLoaded = false;
        }

        UIManager.Instance?.GameplayUI?.Close();
        UIManager.Instance?.MainMenuUI?.Open();
    }
}
