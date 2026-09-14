using System;
using UnityEngine;
using UnityEngine.SceneManagement;

public class SceneLoader : MonoBehaviour
{
    private const string GameSceneName = "GameScene";
    private const string UISceneName = "UIScene";
    private const string AudioSceneName = "AudioScene";

    private static bool _isGameLoadingOrLoaded;

    public static event Action GameSceneLoaded;

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
        LoadScenes();
    }
#else
    private void Start()
    {
        LoadScenes();
    }
#endif

    private static void LoadScenes()
    {
#if UNITY_EDITOR
        if (SceneManager.GetSceneByName(GameSceneName).isLoaded)
            SceneManager.UnloadSceneAsync(GameSceneName);

        if (!SceneManager.GetSceneByName(UISceneName).isLoaded)
            SceneManager.LoadSceneAsync(UISceneName, LoadSceneMode.Additive).completed += _ => OpenMainMenu();
        else
            OpenMainMenu();
        if(!SceneManager.GetSceneByName(AudioSceneName).isLoaded)
            SceneManager.LoadSceneAsync(AudioSceneName, LoadSceneMode.Additive);
#else
        SceneManager.LoadSceneAsync(UISceneName, LoadSceneMode.Additive).completed += _ => OpenMainMenu();
        SceneManager.LoadSceneAsync(AudioSceneName, LoadSceneMode.Additive);
#endif
    }

    public static void StartGame()
    {
        if (_isGameLoadingOrLoaded)
        {
            Debug.LogError("GameScene is already loaded");
            return;
        }
        _isGameLoadingOrLoaded = true;
        SceneManager.LoadSceneAsync(GameSceneName, LoadSceneMode.Additive).completed += _ =>
        {
            SceneManager.SetActiveScene(SceneManager.GetSceneByName(GameSceneName));
            GameSceneLoaded?.Invoke();
        };
        UIManager.Instance.GameplayUI.Open();
        UIManager.Instance.MainMenuUI.Close();
    }

    public static void ResetLevel()
    {
        SceneManager.UnloadSceneAsync(GameSceneName).completed +=
            _ => SceneManager.LoadSceneAsync(GameSceneName, LoadSceneMode.Additive);
    }

    public static void OpenMainMenu()
    {
        // TODO: Possible that scene is loading but not loaded?
        if (SceneManager.GetSceneByName(GameSceneName).isLoaded)
        {
            SceneManager.UnloadSceneAsync(GameSceneName);
            _isGameLoadingOrLoaded = false;
        }

        UIManager.Instance?.GameplayUI?.Close();
        UIManager.Instance?.MainMenuUI?.Open();
    }
}
