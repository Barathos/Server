using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.IO;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using YamlDotNet.Serialization;
using YamlDotNet.Serialization.NamingConventions;
using Microsoft.WindowsAPICodePack.Taskbar;
using System.Diagnostics;
using System.Threading;
using System.Security.Cryptography;

namespace EQEmu_Patcher
{

    public partial class MainForm : Form
    {

        public static string serverName; // server title name
        public static string filelistUrl; //filelist url
        public static string patcherUrl; //patcher url e.g. eqemupatcher-hash.txt
        public static string patchNotesUrl; //patch notes url or base path
        public static string serviceStatusUrl; //service status url or base path
        public static string version; //version of file
        string fileName; //base name of executable
        bool isPatching = false;
        bool isPatchCancelled = false;
        bool isPendingPatch = false; // This is used to indicate that someone pressed "Update" before we did some background update checks
        string myHash; //my MD5 generated hash
        string expectedPatcherHash;
        bool isNeedingSelfUpdate;
        bool isLoading;
        bool isUpdateAvailable = true;
        bool isRestartingForSelfUpdate = false;
        bool isAutoPatch = false;
        bool isAutoPlay = false;
        bool isProjectSelectionLoading = false;
        string configuredFilelistUrl;
        string configuredPatcherUrl;
        string configuredPatchNotesUrl;
        string configuredServiceStatusUrl;
        string clientSuffix;
        List<WorkspacePatchFeed> workspaceFeeds = new List<WorkspacePatchFeed>();
        WorkspacePatchFeed selectedWorkspaceFeed;
        CancellationTokenSource cts;
        System.Diagnostics.Process process;

        //Note that for supported versions, the 3 letter suffix is needed on the filelist_###.yml file.
        public static List<VersionTypes> supportedClients = new List<VersionTypes> { //Supported clients for patcher
            //VersionTypes.Unknown, //unk
            //VersionTypes.Titanium, //tit
            //VersionTypes.Underfoot, //und
            //VersionTypes.Secrets_Of_Feydwer, //sof
            //VersionTypes.Seeds_Of_Destruction, //sod
            VersionTypes.Rain_Of_Fear, //rof
            VersionTypes.Rain_Of_Fear_2 //rof
            //VersionTypes.Broken_Mirror, //bro
        };

        private Dictionary<VersionTypes, ClientVersion> clientVersions = new Dictionary<VersionTypes, ClientVersion>();

        VersionTypes currentVersion;

       // TaskbarItemInfo tii = new TaskbarItemInfo();
        public MainForm()
        {
            InitializeComponent();
        }

        private async void MainForm_Load(object sender, EventArgs e)
        {
            isLoading = true;
            version = Assembly.GetEntryAssembly().GetName().Version.ToString();
            lblVersion.Text = $"Patcher {version}";
            SetLauncherStatus("Checking", "Preparing launcher services.", Color.FromArgb(51, 77, 108));
            SetPrimaryActionChecking();
            Console.WriteLine($"Initializing {version}");
            Console.WriteLine($"Current Directory: {Directory.GetCurrentDirectory()}");
            cts = new CancellationTokenSource();

            serverName = Assembly.GetExecutingAssembly().GetCustomAttribute<ServerName>().Value;
#if (DEBUG)
            serverName = "EQEMU Patcher";
#endif
            if (serverName == "") {
                MessageBox.Show("This patcher was built incorrectly. Please contact the distributor of this and inform them the server name is not provided or screenshot this message.");
                this.Close();
                return;
            }
            lblTitle.Text = serverName;
            lblHeroTitle.Text = serverName;
            this.Text = serverName;

            fileName = Assembly.GetExecutingAssembly().GetCustomAttribute<FileName>().Value;
#if (DEBUG)
            fileName = "eqemupatcher";
#endif
            if (fileName == "")
            {
                MessageBox.Show("This patcher was built incorrectly. Please contact the distributor of this and inform them the file name is not provided or screenshot this message.");
                this.Close();
                return;
            }

            filelistUrl = Assembly.GetExecutingAssembly().GetCustomAttribute<FileListUrl>().Value;
#if (DEBUG)
            filelistUrl = "https://github.com/xackery/eqemupatcher/releases/latest/download";
#endif
            if (filelistUrl == "") {
                MessageBox.Show("This patcher was built incorrectly. Please contact the distributor of this and inform them the file list url is not provided or screenshot this message.", serverName);
                this.Close();
                return;
            }
            if (!filelistUrl.EndsWith("/")) filelistUrl += "/";

            patcherUrl = Assembly.GetExecutingAssembly().GetCustomAttribute<PatcherUrl>().Value;
#if (DEBUG)
            patcherUrl = $"https://github.com/xackery/eqemupatcher/releases/latest/download/";
#endif
            if (patcherUrl == "")
            {
                MessageBox.Show("This patcher was built incorrectly. Please contact the distributor of this and inform them the patcher url is not provided or screenshot this message.", serverName);
                this.Close();
                return;
            }
            if (!patcherUrl.EndsWith("/")) patcherUrl += "/";

            patchNotesUrl = Assembly.GetExecutingAssembly().GetCustomAttribute<PatchNotesUrl>()?.Value ?? "";
#if (DEBUG)
            patchNotesUrl = "https://github.com/xackery/eqemupatcher/releases/latest/download/patch_notes.txt";
#endif

            serviceStatusUrl = Assembly.GetExecutingAssembly().GetCustomAttribute<ServiceStatusUrl>()?.Value ?? "";
#if (DEBUG)
            serviceStatusUrl = "https://github.com/xackery/eqemupatcher/releases/latest/download/patcher_status.yml";
#endif
            configuredFilelistUrl = filelistUrl;
            configuredPatcherUrl = patcherUrl;
            configuredPatchNotesUrl = patchNotesUrl;
            configuredServiceStatusUrl = serviceStatusUrl;

            txtPatchNotes.Text = "Loading patch notes...";
            if (this.Width < 432) {
                this.Width = 432;
            }
            if (this.Height < 550)
            {
                this.Height = 550;
            }
            buildClientVersions();
            IniLibrary.Load();
            detectClientVersion();
            isAutoPlay = (IniLibrary.instance.AutoPlay.ToLower() == "true");
            isAutoPatch = (IniLibrary.instance.AutoPatch.ToLower() == "true");
            chkAutoPlay.Checked = isAutoPlay;
            chkAutoPatch.Checked = isAutoPatch;
            lblLastPatchValue.Text = string.IsNullOrWhiteSpace(IniLibrary.instance.LastPatchedVersion)
                ? "Not patched"
                : ShortVersion(IniLibrary.instance.LastPatchedVersion);
            try
            {
                if (File.Exists(Application.ExecutablePath + ".old"))
                {
                    File.Delete(Application.ExecutablePath + ".old");
                }

            } catch (Exception exDelete)
            {
                Console.WriteLine($"Failed to delete .old file: {exDelete.Message}");
            }

            if (IniLibrary.instance.ClientVersion == VersionTypes.Unknown)
            {
                detectClientVersion();
                if (currentVersion == VersionTypes.Unknown)
                {
                    this.Close();
                }
                IniLibrary.instance.ClientVersion = currentVersion;
                IniLibrary.Save();
            }
            string suffix = "unk";
            if (currentVersion == VersionTypes.Titanium) suffix = "tit";
            if (currentVersion == VersionTypes.Underfoot) suffix = "und";
            if (currentVersion == VersionTypes.Seeds_Of_Destruction) suffix = "sod";
            if (currentVersion == VersionTypes.Broken_Mirror) suffix = "bro";
            if (currentVersion == VersionTypes.Secrets_Of_Feydwer) suffix = "sof";
            if (currentVersion == VersionTypes.Rain_Of_Fear || currentVersion == VersionTypes.Rain_Of_Fear_2) suffix = "rof";

            bool isSupported = false;
            foreach (var ver in supportedClients)
            {
                if (ver != currentVersion) continue;
                isSupported = true;
                break;
            }
            if (!isSupported) {
                SetLauncherStatus("Unsupported", "This client is not supported by the current patch stream.", Color.FromArgb(148, 54, 54));
                MessageBox.Show("The server " + serverName + " does not work with this copy of Everquest (" + currentVersion.ToString().Replace("_", " ") + ")", serverName);
                this.Close();
                return;
            }

            this.Text = serverName + " (Client: " + currentVersion.ToString().Replace("_", " ") + ")";
            lblClientValue.Text = currentVersion.ToString().Replace("_", " ");
            lblSubtitle.Text = $"Detected {lblClientValue.Text}. Checking available patch data.";
            progressBar.Minimum = 0;
            progressBar.Maximum = 10000;
            progressBar.Value = 0;
            StatusLibrary.SubscribeProgress(new StatusLibrary.ProgressHandler((int value) => {
                Invoke((MethodInvoker)delegate {
                    progressBar.Value = value;
                    lblProgress.Text = $"{Math.Min(100, Math.Max(0, value / 100))}%";
                    if (Environment.OSVersion.Version.Major < 6) {
                        return;
                    }
                    var taskbar = TaskbarManager.Instance;
                    taskbar.SetProgressValue(value, 10000);
                    taskbar.SetProgressState((value == 10000) ? TaskbarProgressBarState.NoProgress : TaskbarProgressBarState.Normal);
                });
            }));

            StatusLibrary.SubscribeLogAdd(new StatusLibrary.LogAddHandler((string message) => {
                Invoke((MethodInvoker)delegate {
                    tabContent.SelectedTab = tabPatchLog;
                    txtList.AppendText(message + "\r\n");
                });
            }));

            StatusLibrary.SubscribePatchState(new StatusLibrary.PatchStateHandler((bool isPatchGoing) => {
                Invoke((MethodInvoker)delegate {

                    if (isPatchGoing)
                    {
                        SetPrimaryActionCancel();
                        SetLauncherStatus("Patching", "Updating client files.", Color.FromArgb(203, 154, 70), Color.FromArgb(20, 16, 11));
                        return;
                    }

                    if (isUpdateAvailable || isNeedingSelfUpdate)
                    {
                        SetPrimaryActionUpdate();
                        return;
                    }

                    SetPrimaryActionPlay();
                });
            }));

            clientSuffix = suffix;
            await LoadWorkspaceFeeds(suffix);
            await RefreshActiveFeed(true);
        }

        private async Task LoadWorkspaceFeeds(string suffix)
        {
            workspaceFeeds.Clear();
            selectedWorkspaceFeed = null;
            ApplyProjectSelectorVisibility(false);

            WorkspacePatchersManifest manifest = null;
            foreach (var url in BuildWorkspaceManifestUrls())
            {
                try
                {
                    var data = await Download(cts, url);
                    var body = Encoding.UTF8.GetString(data).Trim();
                    if (body.Length == 0)
                    {
                        continue;
                    }

                    var deserializerBuilder = new DeserializerBuilder().WithNamingConvention(new CamelCaseNamingConvention());
                    var deserializer = deserializerBuilder.Build();
                    using (var input = new StringReader(body))
                    {
                        manifest = deserializer.Deserialize<WorkspacePatchersManifest>(input);
                    }

                    if (manifest != null && manifest.feeds != null && manifest.feeds.Count > 0)
                    {
                        break;
                    }
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"Failed to fetch workspace patcher manifest from {url}: {ex.Message}");
                }
            }

            if (manifest == null || manifest.feeds == null || manifest.feeds.Count == 0)
            {
                ConfigureActiveFeedUrls();
                return;
            }

            foreach (var feed in manifest.feeds)
            {
                if (feed == null || string.IsNullOrWhiteSpace(feed.id))
                {
                    continue;
                }

                if (!string.IsNullOrWhiteSpace(feed.patchClient) &&
                    !feed.patchClient.Equals(suffix, StringComparison.OrdinalIgnoreCase))
                {
                    continue;
                }

                if (string.IsNullOrWhiteSpace(feed.label))
                {
                    feed.label = feed.id;
                }

                if (string.IsNullOrWhiteSpace(feed.feedUrl) && !string.IsNullOrWhiteSpace(manifest.baseUrl))
                {
                    feed.feedUrl = $"{EnsureTrailingSlash(manifest.baseUrl)}{feed.id}/";
                }

                feed.feedUrl = EnsureTrailingSlash(feed.feedUrl);
                workspaceFeeds.Add(feed);
            }

            if (workspaceFeeds.Count == 0)
            {
                ConfigureActiveFeedUrls();
                return;
            }

            selectedWorkspaceFeed = FindInitialWorkspaceFeed();
            if (selectedWorkspaceFeed == null)
            {
                selectedWorkspaceFeed = workspaceFeeds[0];
            }

            isProjectSelectionLoading = true;
            cmbProject.Items.Clear();
            foreach (var feed in workspaceFeeds)
            {
                cmbProject.Items.Add(feed);
            }
            cmbProject.SelectedItem = selectedWorkspaceFeed;
            ApplyProjectSelectorVisibility(workspaceFeeds.Count > 1);
            isProjectSelectionLoading = false;

            IniLibrary.instance.SelectedProject = selectedWorkspaceFeed.id;
            IniLibrary.Save();
            ConfigureActiveFeedUrls();
            ApplySelectedProjectTitle();
        }

        private WorkspacePatchFeed FindInitialWorkspaceFeed()
        {
            if (!string.IsNullOrWhiteSpace(IniLibrary.instance.SelectedProject))
            {
                foreach (var feed in workspaceFeeds)
                {
                    if (feed.id.Equals(IniLibrary.instance.SelectedProject, StringComparison.OrdinalIgnoreCase))
                    {
                        return feed;
                    }
                }
            }

            foreach (var feed in workspaceFeeds)
            {
                if (UrlsEqual(configuredFilelistUrl, feed.feedUrl) || UrlsEqual(configuredPatcherUrl, feed.feedUrl))
                {
                    return feed;
                }
            }

            return null;
        }

        private void ApplyProjectSelectorVisibility(bool visible)
        {
            lblProjectHeading.Visible = visible;
            cmbProject.Visible = visible;
            cmbProject.Enabled = visible && workspaceFeeds.Count > 1;

            var optionX = visible ? 288 : 0;
            chkAutoPatch.Location = new Point(optionX, 43);
            chkAutoPlay.Location = new Point(optionX, 64);
            btnVerifyFiles.Visible = true;
        }

        private async Task RefreshActiveFeed(bool allowAutoActions)
        {
            if (isPatching)
            {
                return;
            }

            isLoading = true;
            isUpdateAvailable = true;
            isNeedingSelfUpdate = false;
            isRestartingForSelfUpdate = false;
            myHash = "";
            expectedPatcherHash = "";
            ConfigureActiveFeedUrls();
            ApplySelectedProjectTitle();
            SetPrimaryActionChecking();
            SetLauncherStatus("Checking", $"Checking {CurrentProjectLabel()} patch data.", Color.FromArgb(51, 77, 108));
            StatusLibrary.SetProgress(0);
            lblProgress.Text = "0%";
            lblManifestValue.Text = "Pending";
            lblDownloadValue.Text = "Pending";
            txtPatchNotes.Text = "Loading patch notes...";

            if (cts != null)
            {
                cts.Cancel();
            }
            cts = new CancellationTokenSource();

            await LoadServiceStatus(clientSuffix);
            await LoadPatchNotes(clientSuffix);

            string response = await DownloadCurrentFileList(clientSuffix);
            if (response != "")
            {
                isLoading = false;
                cts.Cancel();
                SetLauncherStatus("Offline", "Unable to fetch the patch manifest.", Color.FromArgb(148, 54, 54));
                SetPrimaryActionChecking("Offline");
                txtPatchNotes.Text = $"Unable to load the {CurrentProjectLabel()} patch feed.\r\n\r\n{response}";
                StatusLibrary.Log($"Failed to fetch patch manifest: {response}");
                return;
            }

            await CheckSelfUpdate();

            FileList filelist;
            using (var input = File.OpenText($"{System.IO.Path.GetDirectoryName(Application.ExecutablePath)}\\filelist.yml"))
            {
                var deserializerBuilder = new DeserializerBuilder().WithNamingConvention(new CamelCaseNamingConvention());
                var deserializer = deserializerBuilder.Build();
                filelist = deserializer.Deserialize<FileList>(input);
            }
            UpdateManifestSummary(filelist);

            string updateReason;
            bool hasUpdate = !IsClientCurrent(filelist, out updateReason);

            var path = System.IO.Path.GetDirectoryName(Application.ExecutablePath) + "\\eqemupatcher.png";
            if (File.Exists(path))
            {
                splashLogo.Load(path);
            }
            cts.Cancel();
            isLoading = false;

            if (hasUpdate)
            {
                MarkPatchAvailable(updateReason);
                if ((allowAutoActions && isAutoPatch) || isPendingPatch)
                {
                    isPendingPatch = false;
                    pendingPatchTimer.Enabled = false;
                    StartPatch();
                }
                return;
            }

            isPendingPatch = false;
            pendingPatchTimer.Enabled = false;
            MarkReady("Client files are up to date.");
            if (allowAutoActions && isAutoPlay)
            {
                PlayGame();
            }
        }

        private async Task<string> DownloadCurrentFileList(string suffix)
        {
            string webUrl = $"{filelistUrl}{suffix}/filelist_{suffix}.yml";
            string response = await DownloadFile(cts, webUrl, "filelist.yml");
            if (response == "")
            {
                return "";
            }

            webUrl = $"{filelistUrl}filelist_{suffix}.yml";
            return await DownloadFile(cts, webUrl, "filelist.yml");
        }

        private async Task CheckSelfUpdate()
        {
            var updateBaseUrl = GetSelfUpdateBaseUrl();
            string url = $"{updateBaseUrl}{fileName}-hash.txt";
            string response = "";
            try
            {
                var data = await Download(cts, url);
                response = Encoding.Default.GetString(data).Trim().ToUpper();
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Failed to fetch patch from {url}: {ex.Message}");
            }

            if (response != "")
            {
                expectedPatcherHash = response;
                myHash = UtilityLibrary.GetMD5(Application.ExecutablePath);
                if (response != myHash)
                {
                    isNeedingSelfUpdate = true;
                }
            }
        }

        private string GetSelfUpdateBaseUrl()
        {
            foreach (var feed in workspaceFeeds)
            {
                if (feed.id.Equals("patcher", StringComparison.OrdinalIgnoreCase) &&
                    !string.IsNullOrWhiteSpace(feed.feedUrl))
                {
                    return EnsureTrailingSlash(feed.feedUrl);
                }
            }

            return EnsureTrailingSlash(patcherUrl);
        }

        private string GetMD5(byte[] data)
        {
            using (var md5 = MD5.Create())
            {
                var hash = md5.ComputeHash(data);
                var sb = new StringBuilder();
                for (int i = 0; i < hash.Length; i++)
                {
                    sb.Append(hash[i].ToString("X2"));
                }
                return sb.ToString();
            }
        }

        private void RestartLauncherAfterSelfUpdate()
        {
            if (InvokeRequired)
            {
                BeginInvoke((MethodInvoker)delegate {
                    RestartLauncherAfterSelfUpdate();
                });
                return;
            }

            SetPrimaryActionChecking("Restarting...");
            SetLauncherStatus("Updated", "Launcher updated. Restarting now.", Color.FromArgb(79, 185, 119), Color.FromArgb(8, 20, 13));
            try
            {
                Process.Start(Application.ExecutablePath);
                Close();
            }
            catch (Exception ex)
            {
                StatusLibrary.Log($"Launcher restart failed: {ex.Message}");
                SetPrimaryActionChecking("Restart Needed");
                SetLauncherStatus("Updated", "Close and reopen the patcher to finish updating.", Color.FromArgb(224, 181, 92), Color.FromArgb(20, 16, 11));
            }
        }

        private void ConfigureActiveFeedUrls()
        {
            if (selectedWorkspaceFeed == null || string.IsNullOrWhiteSpace(selectedWorkspaceFeed.feedUrl))
            {
                filelistUrl = EnsureTrailingSlash(configuredFilelistUrl);
                patcherUrl = EnsureTrailingSlash(configuredPatcherUrl);
                patchNotesUrl = configuredPatchNotesUrl;
                serviceStatusUrl = configuredServiceStatusUrl;
                return;
            }

            var feedUrl = EnsureTrailingSlash(selectedWorkspaceFeed.feedUrl);
            filelistUrl = feedUrl;
            patcherUrl = feedUrl;
            patchNotesUrl = $"{feedUrl}patch_notes.txt";
            serviceStatusUrl = $"{feedUrl}patcher_status.yml";
        }

        private void ApplySelectedProjectTitle()
        {
            bool isProjectSwitcher = selectedWorkspaceFeed != null && workspaceFeeds.Count > 1;
            string title = isProjectSwitcher
                ? $"{selectedWorkspaceFeed.label} Test Patcher"
                : serverName;
            lblTitle.Text = title;
            lblHeroTitle.Text = title;
            lblHeroKicker.Text = isProjectSwitcher ? "PROJECT PATCH FEED" : "LIVE PATCH FEED";
            lblHeroSubtitle.Text = isProjectSwitcher
                ? "Choose a test project, sync only the files it needs, and launch with conflicts cleaned up."
                : "Stay current, read the latest update notes, and launch directly into Norrath.";
            if (!string.IsNullOrWhiteSpace(clientSuffix))
            {
                this.Text = title + " (Client: " + currentVersion.ToString().Replace("_", " ") + ")";
            }
            else
            {
                this.Text = title;
            }
        }

        private string CurrentProjectLabel()
        {
            return selectedWorkspaceFeed == null ? serverName : selectedWorkspaceFeed.label;
        }

        private IEnumerable<string> BuildWorkspaceManifestUrls()
        {
            var seen = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            foreach (var source in new[] { configuredFilelistUrl, configuredPatcherUrl })
            {
                foreach (var baseUrl in new[] { EnsureTrailingSlash(source), GetParentUrl(source) })
                {
                    foreach (var manifestName in new[] { "workspace_patchers.yml", "workspace_patchers.yaml", "workspace_patchers.json" })
                    {
                        if (string.IsNullOrWhiteSpace(baseUrl))
                        {
                            continue;
                        }

                        var url = EnsureTrailingSlash(baseUrl) + manifestName;
                        if (seen.Add(url))
                        {
                            yield return url;
                        }
                    }
                }
            }
        }

        private string EnsureTrailingSlash(string value)
        {
            if (string.IsNullOrWhiteSpace(value))
            {
                return "";
            }

            value = value.Trim();
            return value.EndsWith("/") ? value : value + "/";
        }

        private string GetParentUrl(string value)
        {
            if (string.IsNullOrWhiteSpace(value))
            {
                return "";
            }

            try
            {
                var uri = new Uri(EnsureTrailingSlash(value));
                var path = uri.AbsolutePath.TrimEnd('/');
                var index = path.LastIndexOf('/');
                if (index <= 0)
                {
                    return EnsureTrailingSlash(value);
                }

                var builder = new UriBuilder(uri);
                builder.Path = path.Substring(0, index + 1);
                builder.Query = "";
                builder.Fragment = "";
                return builder.Uri.ToString();
            }
            catch
            {
                return EnsureTrailingSlash(value);
            }
        }

        private bool UrlsEqual(string left, string right)
        {
            return EnsureTrailingSlash(left).Equals(EnsureTrailingSlash(right), StringComparison.OrdinalIgnoreCase);
        }

        private async void cmbProject_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (isProjectSelectionLoading)
            {
                return;
            }

            var nextFeed = cmbProject.SelectedItem as WorkspacePatchFeed;
            if (nextFeed == null || nextFeed == selectedWorkspaceFeed)
            {
                return;
            }

            if (isPatching)
            {
                MessageBox.Show("Wait for the current patch to finish before switching test projects.", serverName);
                isProjectSelectionLoading = true;
                cmbProject.SelectedItem = selectedWorkspaceFeed;
                isProjectSelectionLoading = false;
                return;
            }

            selectedWorkspaceFeed = nextFeed;
            IniLibrary.instance.SelectedProject = selectedWorkspaceFeed.id;
            IniLibrary.instance.LastPatchedVersion = "";
            IniLibrary.Save();
            lblLastPatchValue.Text = "Not patched";
            StatusLibrary.Log($"Selected test project: {selectedWorkspaceFeed.label}");
            await RefreshActiveFeed(false);
        }

        private async Task LoadPatchNotes(string suffix)
        {
            foreach (var url in BuildPatchNotesUrls(suffix))
            {
                try
                {
                    var data = await Download(cts, url);
                    var body = Encoding.UTF8.GetString(data).Trim();
                    if (body.Length == 0)
                    {
                        continue;
                    }

                    ApplyPatchNotes(body);
                    tabContent.SelectedTab = tabPatchNotes;
                    return;
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"Failed to fetch patch notes from {url}: {ex.Message}");
                }
            }

            txtPatchNotes.Text = "Patch notes are not available yet.\r\n\r\nCheck back after the next patch publish.";
            txtPatchNotes.SelectionStart = 0;
            txtPatchNotes.SelectionLength = 0;
        }

        private async Task LoadServiceStatus(string suffix)
        {
            foreach (var url in BuildServiceStatusUrls(suffix))
            {
                try
                {
                    var data = await Download(cts, url);
                    var body = Encoding.UTF8.GetString(data).Trim();
                    if (body.Length == 0)
                    {
                        continue;
                    }

                    var deserializerBuilder = new DeserializerBuilder().WithNamingConvention(new CamelCaseNamingConvention());
                    var deserializer = deserializerBuilder.Build();
                    using (var input = new StringReader(body))
                    {
                        var status = deserializer.Deserialize<PatcherServiceStatus>(input);
                        ApplyServiceStatus(status);
                    }
                    return;
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"Failed to fetch patcher status from {url}: {ex.Message}");
                }
            }

            lblServiceStatus.Text = "Unknown";
            lblServiceStatus.ForeColor = Color.FromArgb(224, 181, 92);
            lblServiceMessage.Text = "Patch service status has not been published yet.";
        }

        private IEnumerable<string> BuildServiceStatusUrls(string suffix)
        {
            var seen = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            foreach (var url in BuildStatusUrlsFromSource(serviceStatusUrl, suffix))
            {
                if (seen.Add(url))
                {
                    yield return url;
                }
            }

            foreach (var url in BuildStatusUrlsFromSource(filelistUrl, suffix))
            {
                if (seen.Add(url))
                {
                    yield return url;
                }
            }
        }

        private IEnumerable<string> BuildStatusUrlsFromSource(string source, string suffix)
        {
            if (string.IsNullOrWhiteSpace(source))
            {
                yield break;
            }

            source = source.Trim();
            if (LooksLikeStatusFileUrl(source))
            {
                yield return source;
                yield break;
            }

            if (!source.EndsWith("/"))
            {
                source += "/";
            }

            yield return $"{source}patcher_status.yml";
            yield return $"{source}status.yml";
            yield return $"{source}{suffix}/patcher_status.yml";
            yield return $"{source}{suffix}/status.yml";
        }

        private IEnumerable<string> BuildPatchNotesUrls(string suffix)
        {
            var seen = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            foreach (var url in BuildPatchNotesUrlsFromSource(patchNotesUrl, suffix))
            {
                if (seen.Add(url))
                {
                    yield return url;
                }
            }

            foreach (var url in BuildPatchNotesUrlsFromSource(filelistUrl, suffix))
            {
                if (seen.Add(url))
                {
                    yield return url;
                }
            }
        }

        private IEnumerable<string> BuildPatchNotesUrlsFromSource(string source, string suffix)
        {
            if (string.IsNullOrWhiteSpace(source))
            {
                yield break;
            }

            source = source.Trim();
            if (LooksLikePatchNotesFileUrl(source))
            {
                yield return source;
                yield break;
            }

            if (!source.EndsWith("/"))
            {
                source += "/";
            }

            yield return $"{source}{suffix}/patch_notes.txt";
            yield return $"{source}{suffix}/patch-notes.txt";
            yield return $"{source}patch_notes.txt";
            yield return $"{source}patch-notes.txt";
        }

        private bool LooksLikePatchNotesFileUrl(string url)
        {
            Uri uri;
            if (!Uri.TryCreate(url, UriKind.Absolute, out uri))
            {
                return false;
            }

            var fileName = Path.GetFileName(uri.LocalPath);
            if (string.IsNullOrWhiteSpace(fileName))
            {
                return false;
            }

            var extension = Path.GetExtension(fileName);
            return extension.Equals(".txt", StringComparison.OrdinalIgnoreCase) ||
                extension.Equals(".md", StringComparison.OrdinalIgnoreCase) ||
                extension.Equals(".html", StringComparison.OrdinalIgnoreCase) ||
                extension.Equals(".htm", StringComparison.OrdinalIgnoreCase);
        }

        private bool LooksLikeStatusFileUrl(string url)
        {
            Uri uri;
            if (!Uri.TryCreate(url, UriKind.Absolute, out uri))
            {
                return false;
            }

            var fileName = Path.GetFileName(uri.LocalPath);
            if (string.IsNullOrWhiteSpace(fileName))
            {
                return false;
            }

            var extension = Path.GetExtension(fileName);
            return extension.Equals(".yml", StringComparison.OrdinalIgnoreCase) ||
                extension.Equals(".yaml", StringComparison.OrdinalIgnoreCase);
        }

        private string NormalizePatchNotes(string body)
        {
            return body.Replace("\r\n", "\n").Replace("\n", "\r\n");
        }

        private void ApplyPatchNotes(string body)
        {
            txtPatchNotes.Clear();
            foreach (var rawLine in NormalizePatchNotes(body).Split(new[] { "\r\n" }, StringSplitOptions.None))
            {
                var line = rawLine.TrimEnd();
                var trimmed = line.TrimStart();
                if (trimmed.StartsWith("#") || (trimmed.EndsWith(":") && trimmed.Length < 80))
                {
                    AppendPatchNoteLine(line.TrimStart('#', ' '), Color.FromArgb(224, 181, 92), new Font(txtPatchNotes.Font, FontStyle.Bold));
                }
                else if (trimmed.StartsWith("-") || trimmed.StartsWith("*"))
                {
                    AppendPatchNoteLine(line, Color.FromArgb(225, 231, 239), txtPatchNotes.Font);
                }
                else
                {
                    AppendPatchNoteLine(line, Color.FromArgb(196, 205, 218), txtPatchNotes.Font);
                }
            }

            txtPatchNotes.SelectionStart = 0;
            txtPatchNotes.SelectionLength = 0;
        }

        private void AppendPatchNoteLine(string line, Color color, Font font)
        {
            txtPatchNotes.SelectionStart = txtPatchNotes.TextLength;
            txtPatchNotes.SelectionLength = 0;
            txtPatchNotes.SelectionColor = color;
            txtPatchNotes.SelectionFont = font;
            txtPatchNotes.AppendText(line + "\r\n");
            txtPatchNotes.SelectionFont = txtPatchNotes.Font;
            txtPatchNotes.SelectionColor = txtPatchNotes.ForeColor;
        }

        private void ApplyServiceStatus(PatcherServiceStatus status)
        {
            if (status == null)
            {
                lblServiceStatus.Text = "Unknown";
                lblServiceStatus.ForeColor = Color.FromArgb(224, 181, 92);
                lblServiceMessage.Text = "Patch service status could not be parsed.";
                return;
            }

            var value = string.IsNullOrWhiteSpace(status.status) ? "Online" : status.status.Trim();
            lblServiceStatus.Text = value;
            lblServiceStatus.ForeColor = value.Equals("online", StringComparison.OrdinalIgnoreCase)
                ? Color.FromArgb(116, 226, 158)
                : Color.FromArgb(224, 181, 92);
            lblServiceMessage.Text = string.IsNullOrWhiteSpace(status.message)
                ? "Patch service is reachable."
                : status.message.Trim();

            if (!string.IsNullOrWhiteSpace(status.motd))
            {
                lblSubtitle.Text = status.motd.Trim();
            }
        }

        private void UpdateManifestSummary(FileList filelist)
        {
            if (filelist == null)
            {
                lblManifestValue.Text = "Unavailable";
                lblDownloadValue.Text = "Unavailable";
                return;
            }

            var downloads = filelist.downloads ?? new List<FileEntry>();
            long totalBytes = 0;
            foreach (var entry in downloads)
            {
                totalBytes += entry.size;
            }

            lblManifestValue.Text = string.IsNullOrWhiteSpace(filelist.version)
                ? $"{downloads.Count} files"
                : ShortVersion(filelist.version);
            lblDownloadValue.Text = $"{generateSize(totalBytes)} / {downloads.Count} files";
        }

        private bool IsClientCurrent(FileList filelist, out string reason)
        {
            reason = "Client files are up to date.";

            if (isNeedingSelfUpdate)
            {
                reason = "Launcher update available.";
                return false;
            }

            if (filelist == null)
            {
                reason = "Patch manifest is unavailable.";
                return false;
            }

            var basePath = Path.GetDirectoryName(Application.ExecutablePath);
            var downloads = filelist.downloads ?? new List<FileEntry>();
            foreach (var entry in downloads)
            {
                if (entry == null || string.IsNullOrWhiteSpace(entry.name))
                {
                    continue;
                }

                var path = Path.Combine(basePath, entry.name.Replace("/", "\\"));
                if (!UtilityLibrary.IsPathChild(path))
                {
                    continue;
                }

                if (!File.Exists(path))
                {
                    reason = $"{entry.name} is missing.";
                    return false;
                }

                if (!string.IsNullOrWhiteSpace(entry.md5))
                {
                    var md5 = UtilityLibrary.GetMD5(path);
                    if (!string.Equals(md5, entry.md5, StringComparison.OrdinalIgnoreCase))
                    {
                        reason = $"{entry.name} needs update.";
                        return false;
                    }
                }
            }

            var deletes = filelist.deletes ?? new List<FileEntry>();
            foreach (var entry in deletes)
            {
                if (entry == null || string.IsNullOrWhiteSpace(entry.name))
                {
                    continue;
                }

                var path = Path.Combine(basePath, entry.name.Replace("/", "\\"));
                if (!UtilityLibrary.IsPathChild(path))
                {
                    continue;
                }

                if (File.Exists(path))
                {
                    reason = $"{entry.name} should be removed.";
                    return false;
                }
            }

            if (!string.Equals(filelist.version, IniLibrary.instance.LastPatchedVersion, StringComparison.OrdinalIgnoreCase))
            {
                IniLibrary.instance.LastPatchedVersion = filelist.version;
                IniLibrary.Save();
                SetLastPatchedLabel(filelist.version);
            }

            return true;
        }

        private void SetPrimaryActionChecking()
        {
            SetPrimaryActionChecking("Checking...");
        }

        private void SetPrimaryActionChecking(string text)
        {
            if (InvokeRequired)
            {
                BeginInvoke((MethodInvoker)delegate {
                    SetPrimaryActionChecking(text);
                });
                return;
            }

            btnStart.Visible = false;
            btnStart.Enabled = false;
            btnVerifyFiles.Enabled = false;
            btnCheck.Enabled = false;
            btnCheck.Text = text;
            btnCheck.BackColor = Color.FromArgb(51, 77, 108);
            btnCheck.ForeColor = Color.White;
        }

        private void SetPrimaryActionUpdate()
        {
            if (InvokeRequired)
            {
                BeginInvoke((MethodInvoker)delegate {
                    SetPrimaryActionUpdate();
                });
                return;
            }

            btnStart.Visible = false;
            btnStart.Enabled = false;
            btnVerifyFiles.Enabled = true;
            btnCheck.Enabled = true;
            btnCheck.Text = "Update";
            btnCheck.BackColor = Color.FromArgb(224, 181, 92);
            btnCheck.ForeColor = Color.FromArgb(20, 16, 11);
        }

        private void SetPrimaryActionPlay()
        {
            if (InvokeRequired)
            {
                BeginInvoke((MethodInvoker)delegate {
                    SetPrimaryActionPlay();
                });
                return;
            }

            btnStart.Visible = false;
            btnStart.Enabled = false;
            btnVerifyFiles.Enabled = true;
            btnCheck.Enabled = true;
            btnCheck.Text = "Play";
            btnCheck.BackColor = Color.FromArgb(79, 185, 119);
            btnCheck.ForeColor = Color.FromArgb(8, 20, 13);
        }

        private void SetPrimaryActionCancel()
        {
            if (InvokeRequired)
            {
                BeginInvoke((MethodInvoker)delegate {
                    SetPrimaryActionCancel();
                });
                return;
            }

            btnStart.Visible = false;
            btnStart.Enabled = false;
            btnVerifyFiles.Enabled = false;
            btnCheck.Enabled = true;
            btnCheck.Text = "Cancel";
            btnCheck.BackColor = Color.FromArgb(148, 54, 54);
            btnCheck.ForeColor = Color.White;
        }

        private void MarkReady(string subtitle)
        {
            isUpdateAvailable = false;
            SetPrimaryActionPlay();
            SetLauncherStatus("Ready", subtitle, Color.FromArgb(79, 185, 119), Color.FromArgb(8, 20, 13));
        }

        private void MarkPatchAvailable(string subtitle)
        {
            isUpdateAvailable = true;
            if (isPatching)
            {
                SetPrimaryActionCancel();
            }
            else
            {
                SetPrimaryActionUpdate();
            }

            SetLauncherStatus("Update Ready", subtitle, Color.FromArgb(224, 181, 92), Color.FromArgb(20, 16, 11));
        }

        private void SetLauncherStatus(string status, string subtitle, Color backColor)
        {
            SetLauncherStatus(status, subtitle, backColor, Color.White);
        }

        private void SetLauncherStatus(string status, string subtitle, Color backColor, Color foreColor)
        {
            if (InvokeRequired)
            {
                BeginInvoke((MethodInvoker)delegate {
                    SetLauncherStatus(status, subtitle, backColor, foreColor);
                });
                return;
            }

            lblStatusBadge.Text = status;
            lblStatusBadge.BackColor = backColor;
            lblStatusBadge.ForeColor = foreColor;
            lblSubtitle.Text = subtitle;
        }

        private void SetLastPatchedLabel(string versionValue)
        {
            if (InvokeRequired)
            {
                BeginInvoke((MethodInvoker)delegate {
                    SetLastPatchedLabel(versionValue);
                });
                return;
            }

            lblLastPatchValue.Text = ShortVersion(versionValue);
        }

        private string ShortVersion(string value)
        {
            if (string.IsNullOrWhiteSpace(value))
            {
                return "Unknown";
            }

            value = value.Trim();
            return value.Length > 16 ? value.Substring(0, 16) : value;
        }

        private void heroPanel_Paint(object sender, PaintEventArgs e)
        {
            using (var brush = new LinearGradientBrush(heroPanel.ClientRectangle, Color.FromArgb(10, 14, 22), Color.FromArgb(42, 51, 66), 90F))
            {
                e.Graphics.FillRectangle(brush, heroPanel.ClientRectangle);
            }

            using (var accent = new SolidBrush(Color.FromArgb(42, Color.FromArgb(224, 181, 92))))
            {
                e.Graphics.FillEllipse(accent, -90, 120, 260, 260);
                e.Graphics.FillEllipse(accent, 150, -80, 220, 220);
            }
        }

        private void detectClientVersion()
        {
            try
            {

                var hash = UtilityLibrary.GetEverquestExecutableHash(AppDomain.CurrentDomain.BaseDirectory);
                if (hash == "")
                {
                    MessageBox.Show("Please run this patcher in your Everquest directory.");
                    this.Close();
                    return;
                }
                switch (hash)
                {
                    case "85218FC053D8B367F2B704BAC5E30ACC":
                        currentVersion = VersionTypes.Secrets_Of_Feydwer;
                        splashLogo.Image = Properties.Resources.sof;
                        break;
                    case "859E89987AA636D36B1007F11C2CD6E0":
                    case "EF07EE6649C9A2BA2EFFC3F346388E1E78B44B48": //one of the torrented uf clients, used by B&R too
                        currentVersion = VersionTypes.Underfoot;
                        splashLogo.Image = Properties.Resources.underfoot;
                        break;
                    case "A9DE1B8CC5C451B32084656FCACF1103": //p99 client
                    case "BB42BC3870F59B6424A56FED3289C6D4": //vanilla titanium
                        currentVersion = VersionTypes.Titanium;
                        splashLogo.Image = Properties.Resources.titanium;
                        break;
                    case "368BB9F425C8A55030A63E606D184445":
                        currentVersion = VersionTypes.Rain_Of_Fear;
                        splashLogo.Image = Properties.Resources.rof;
                        break;
                    case "240C80800112ADA825C146D7349CE85B":
                    case "A057A23F030BAA1C4910323B131407105ACAD14D": //This is a custom ROF2 from a torrent download
                    case "389709EC0E456C3DAE881A61218AAB3F": // This is a 4gb patched eqgame
                    case "6574AC667D4C522D21A47F4D00920CC2": // Unknown origin, issue #29
                    case "AE4E4C995DF8842DAE3127E88E724033": // gangsta of RoT 4gb patched eqgame
                    case "3B44C6CD42313CB80C323647BCB296EF": //https://github.com/xackery/eqemupatcher/issues/15
                    case "513FDC2B5CC63898D7962F0985D5C207": //aslr checksum removed
                    case "2FD5E6243BCC909D9FD0587A156A1165": //https://github.com/xackery/eqemupatcher/issues/20
                    case "26DC13388395A20B73E1B5A08415B0F8": //Legacy of Norrath Custom RoF2 Client https://github.com/xackery/eqemupatcher/issues/16
                        currentVersion = VersionTypes.Rain_Of_Fear_2;
                        splashLogo.Image = Properties.Resources.rof;
                        break;
                    case "6BFAE252C1A64FE8A3E176CAEE7AAE60": //This is one of the live EQ binaries.
                    case "AD970AD6DB97E5BB21141C205CAD6E68": //2016/08/27
                        currentVersion = VersionTypes.Broken_Mirror;
                        splashLogo.Image = Properties.Resources.brokenmirror;
                        break;
                    default:
                        currentVersion = VersionTypes.Unknown;
                        break;
                }
                if (currentVersion == VersionTypes.Unknown)
                {
                    if (MessageBox.Show("Unable to recognize the Everquest client in this directory, open a web page to report to devs?", "Visit", MessageBoxButtons.YesNo, MessageBoxIcon.Asterisk) == DialogResult.Yes)
                    {
                        System.Diagnostics.Process.Start("https://github.com/Xackery/eqemupatcher/issues/new?title=A+New+EQClient+Found&body=Hi+I+Found+A+New+Client!+Hash:+" + hash);
                    }
                    StatusLibrary.Log($"Unable to recognize the Everquest client in this directory, send to developers: {hash}");
                }
                else
                {
                    //StatusLibrary.Log($"You seem to have put me in a {clientVersions[currentVersion].FullName} client directory");
                }

                //MessageBox.Show(""+currentVersion);
                //StatusLibrary.Log($"If you wish to help out, press the scan button on the bottom left and wait for it to complete, then copy paste this data as an Issue on github!");
            }
            catch (UnauthorizedAccessException err)
            {
                MessageBox.Show("You need to run this program with Administrative Privileges" + err.Message);
                return;
            }
        }

        //Build out all client version's dictionary
        private void buildClientVersions()
        {
            clientVersions.Clear();
            clientVersions.Add(VersionTypes.Titanium, new ClientVersion("Titanium", "titanium"));
            clientVersions.Add(VersionTypes.Secrets_Of_Feydwer, new ClientVersion("Secrets Of Feydwer", "sof"));
            clientVersions.Add(VersionTypes.Seeds_Of_Destruction, new ClientVersion("Seeds of Destruction", "sod"));
            clientVersions.Add(VersionTypes.Rain_Of_Fear, new ClientVersion("Rain of Fear", "rof"));
            clientVersions.Add(VersionTypes.Rain_Of_Fear_2, new ClientVersion("Rain of Fear 2", "rof2"));
            clientVersions.Add(VersionTypes.Underfoot, new ClientVersion("Underfoot", "underfoot"));
            clientVersions.Add(VersionTypes.Broken_Mirror, new ClientVersion("Broken Mirror", "brokenmirror"));
        }


        private void btnStart_Click(object sender, EventArgs e)
        {
            btnCheck_Click(sender, e);
        }

        private void PlayGame()
        {
            if (InvokeRequired)
            {
                BeginInvoke((MethodInvoker)delegate {
                    PlayGame();
                });
                return;
            }

            if (isLoading)
            {
                StatusLibrary.Log("Please wait while the patcher checks for updates.");
                SetLauncherStatus("Checking", "Checking for required updates before play.", Color.FromArgb(51, 77, 108));
                SetPrimaryActionChecking();
                return;
            }

            if (isPatching)
            {
                StatusLibrary.Log("Patching is still in progress. Please wait for it to finish.");
                SetLauncherStatus("Patching", "Updating client files before play.", Color.FromArgb(203, 154, 70), Color.FromArgb(20, 16, 11));
                SetPrimaryActionCancel();
                return;
            }

            if (isUpdateAvailable || isNeedingSelfUpdate)
            {
                StatusLibrary.Log("Update required before play.");
                MarkPatchAvailable("Update required before play.");
                return;
            }

            try
            {
                process = UtilityLibrary.StartEverquest();
                if (process != null) this.Close();
                else MessageBox.Show("The process failed to start");
            }
            catch (Exception err)
            {
                MessageBox.Show("An error occured while trying to start everquest: " + err.Message);
            }
        }


        private void btnCheck_Click(object sender, EventArgs e)
        {
            if (isLoading)
            {
                isPendingPatch = true;
                pendingPatchTimer.Enabled = true;
                StatusLibrary.Log("Still checking for updates...");
                SetPrimaryActionChecking();
                return;
            }

            if (isPatching)
            {
                isPatchCancelled = true;
                cts.Cancel();
                SetLauncherStatus("Cancelling", "Stopping after the current file operation.", Color.FromArgb(148, 54, 54));
                StatusLibrary.Log("Cancellation requested.");
                return;
            }

            if (isUpdateAvailable || isNeedingSelfUpdate)
            {
                StartPatch();
                return;
            }

            PlayGame();
        }

        private void btnVerifyFiles_Click(object sender, EventArgs e)
        {
            if (isLoading)
            {
                IniLibrary.instance.LastPatchedVersion = "";
                IniLibrary.Save();
                SetLastPatchedLabel("");
                isPendingPatch = true;
                pendingPatchTimer.Enabled = true;
                StatusLibrary.Log("Verify requested. Waiting for patch manifest...");
                SetPrimaryActionChecking("Verifying...");
                return;
            }

            if (isPatching)
            {
                StatusLibrary.Log("A patch is already running.");
                return;
            }

            IniLibrary.instance.LastPatchedVersion = "";
            IniLibrary.Save();
            SetLastPatchedLabel("");
            StatusLibrary.Log("Verifying files against the current patch manifest...");
            SetLauncherStatus("Verifying", "Checking and repairing client files.", Color.FromArgb(51, 77, 108));
            isUpdateAvailable = true;
            StartPatch();
        }

        public static async Task<string> DownloadFile(CancellationTokenSource cts, string url, string path)
        {
            path = path.Replace("/", "\\");
            if (path.Contains("\\")) { //Make directory if needed.
                string dir = System.IO.Path.GetDirectoryName(Application.ExecutablePath) + "\\" + path.Substring(0, path.LastIndexOf("\\"));
                Directory.CreateDirectory(dir);
            }
            return await UtilityLibrary.DownloadFile(cts, url, path);
        }

        public static async Task<byte[]> Download(CancellationTokenSource cts, string url)
        {
            return await UtilityLibrary.Download(cts, url);
        }

        private void StartPatch()
        {
            if (isPatching)
            {
                Console.WriteLine("premature patch call");
                return;
            }
            cts = new CancellationTokenSource();
            isPatchCancelled = false;
            txtList.Text = "";
            tabContent.SelectedTab = tabPatchLog;
            SetLauncherStatus("Patching", "Preparing patch operations.", Color.FromArgb(203, 154, 70), Color.FromArgb(20, 16, 11));
            isPatching = true;
            isUpdateAvailable = true;
            StatusLibrary.SetPatchState(true);
            Task.Run(async () =>
            {
                bool patchSucceeded = false;
                bool wasCancelled = false;
                try
                {
                    patchSucceeded = await AsyncPatch();
                } catch (Exception e)
                {
                    StatusLibrary.Log($"Exception during patch: {e.Message}");
                }

                wasCancelled = isPatchCancelled;
                isUpdateAvailable = !patchSucceeded;
                isPatching = false;
                isPatchCancelled = false;
                StatusLibrary.SetPatchState(false);
                cts.Cancel();

                if (isRestartingForSelfUpdate)
                {
                    return;
                }

                if (patchSucceeded)
                {
                    if (isAutoPlay) PlayGame();
                    return;
                }

                MarkPatchAvailable(wasCancelled
                    ? "Patch cancelled. Update required before play."
                    : "Patch did not complete. Update required before play.");
            });
        }

        private async Task<bool> AsyncPatch()
        {
            Stopwatch start = Stopwatch.StartNew();
            StatusLibrary.Log($"Patching with patcher version {version}...");
            StatusLibrary.SetProgress(0);
            FileList filelist;

            using (var input = File.OpenText($"{System.IO.Path.GetDirectoryName(Application.ExecutablePath)}\\filelist.yml"))
            {
                var deserializerBuilder = new DeserializerBuilder().WithNamingConvention(new CamelCaseNamingConvention());

                var deserializer = deserializerBuilder.Build();

                filelist = deserializer.Deserialize<FileList>(input);
            }

            double totalBytes = 0; //total patch size
            double currentBytes = 1; // current patched size
            double patchedBytes = 0; // how many files patched size

            var downloads = filelist.downloads ?? new List<FileEntry>();
            foreach (var entry in downloads)
            {
                totalBytes += entry.size;
            }
            if (totalBytes == 0) totalBytes = 1;

            if (!string.IsNullOrWhiteSpace(myHash) && isNeedingSelfUpdate)
            {
                StatusLibrary.Log("Launcher update needed, downloading the newest patcher...");
                string url = $"{GetSelfUpdateBaseUrl()}{fileName}.exe";
                var executablePath = Application.ExecutablePath;
                var newPath = executablePath + ".new";
                var oldPath = executablePath + ".old";
                try
                {
                    var data = await Download(cts, url);
                    var downloadedHash = GetMD5(data);
                    if (!string.IsNullOrWhiteSpace(expectedPatcherHash) &&
                        !downloadedHash.Equals(expectedPatcherHash, StringComparison.OrdinalIgnoreCase))
                    {
                        StatusLibrary.Log($"Launcher update hash mismatch from {url}: expected {expectedPatcherHash}, got {downloadedHash}.");
                        MarkPatchAvailable("Launcher update hash mismatch.");
                        return false;
                    }

                    if (File.Exists(newPath))
                    {
                        File.Delete(newPath);
                    }
                    using (var w = File.Create(newPath))
                    {
                        await w.WriteAsync(data, 0, data.Length, cts.Token);
                    }
                    if (File.Exists(oldPath))
                    {
                        File.Delete(oldPath);
                    }
                    File.Move(executablePath, oldPath);
                    File.Move(newPath, executablePath);
                    StatusLibrary.Log($"Launcher updated ({generateSize(data.Length)}). Restarting to finish the update.");
                    isNeedingSelfUpdate = false;
                    isRestartingForSelfUpdate = true;
                    RestartLauncherAfterSelfUpdate();
                    return true;

                } catch (Exception e)
                {
                    try
                    {
                        if (!File.Exists(executablePath) && File.Exists(oldPath))
                        {
                            File.Move(oldPath, executablePath);
                        }
                    }
                    catch (Exception restoreException)
                    {
                        StatusLibrary.Log($"Launcher rollback failed: {restoreException.Message}");
                    }
                    StatusLibrary.Log($"Launcher update failed {url}: {e.Message}");
                    isNeedingSelfUpdate = true;
                    MarkPatchAvailable("Launcher update required.");
                    return false;
                }
            }
            if (string.IsNullOrWhiteSpace(filelist.downloadprefix))
            {
                StatusLibrary.Log("Patch manifest is missing downloadprefix.");
                SetLauncherStatus("Manifest Error", "The patch manifest is missing its download prefix.", Color.FromArgb(148, 54, 54));
                return false;
            }
            if (!filelist.downloadprefix.EndsWith("/")) filelist.downloadprefix += "/";
            foreach (var entry in downloads)
            {
                if (isPatchCancelled)
                {
                    Console.WriteLine("cancelled while downloading");
                    StatusLibrary.Log("Patching cancelled.");
                    return false;
                }

                StatusLibrary.SetProgress((int)(currentBytes / totalBytes * 10000));

                var path = Path.GetDirectoryName(Application.ExecutablePath)+"\\"+entry.name.Replace("/", "\\");
                if (!UtilityLibrary.IsPathChild(path))
                {
                    StatusLibrary.Log("Path " + path + " might be outside of your Everquest directory. Skipping download to this location.");
                    continue;
                }

                // check if file exists and is already patched
                if (File.Exists(path)) {
                    var md5 = UtilityLibrary.GetMD5(path);
                    if (md5.ToUpper() == entry.md5.ToUpper())
                    {
                        currentBytes += entry.size;
                        continue;
                    }
                    Console.WriteLine($"{path} {md5} vs {entry.md5}");
                }


                string url = filelist.downloadprefix + entry.name.Replace("\\", "/");

                string resp = await DownloadFile(cts, url, entry.name);
                if (resp != "")
                {
                    if (resp == "404")
                    {
                        StatusLibrary.Log($"Failed to download {entry.name} ({generateSize(entry.size)}) from {url}, 404 error (website may be down?)");
                        return false;
                    }
                    StatusLibrary.Log($"Failed to download {entry.name} ({generateSize(entry.size)}) from {url}: {resp}");
                    return false;
                }
                StatusLibrary.Log($"{entry.name} ({generateSize(entry.size)})");

                currentBytes += entry.size;
                patchedBytes += entry.size;
            }

            if (filelist.deletes != null && filelist.deletes.Count > 0)
            {
                foreach (var entry in filelist.deletes)
                {
                    var path = Path.GetDirectoryName(Application.ExecutablePath) + "\\" + entry.name.Replace("/", "\\");
                    if (isPatchCancelled)
                    {
                        Console.WriteLine("cancellled while deleting");
                        StatusLibrary.Log("Patching cancelled.");
                        return false;
                    }
                    if (!UtilityLibrary.IsPathChild(path))
                    {
                        StatusLibrary.Log("Path " + entry.name + " might be outside your Everquest directory. Skipping deletion of this file.");
                        continue;
                    }
                    if (File.Exists(path))
                    {
                        StatusLibrary.Log("Deleting " + entry.name + "...");
                        File.Delete(path);
                    }
                }
            }

            StatusLibrary.SetProgress(10000);
            if (patchedBytes == 0)
            {
                string version = ShortVersion(filelist.version);

                StatusLibrary.Log($"Up to date with patch {version}.");
                IniLibrary.instance.LastPatchedVersion = filelist.version;
                IniLibrary.Save();
                SetLastPatchedLabel(filelist.version);
                MarkReady("Client files are up to date.");
                return true;
            }

            string elapsed = start.Elapsed.ToString("ss\\.ff");
            StatusLibrary.Log($"Complete! Patched {generateSize(patchedBytes)} in {elapsed} seconds. Press Play to begin.");
            IniLibrary.instance.LastPatchedVersion = filelist.version;
            IniLibrary.Save();
            SetLastPatchedLabel(filelist.version);
            MarkReady("Patch complete. You are ready to play.");
            return true;
        }

        private void chkAutoPlay_CheckedChanged(object sender, EventArgs e)
        {
            if (isLoading) return;
            isAutoPlay = chkAutoPlay.Checked;
            IniLibrary.instance.AutoPlay = (isAutoPlay) ? "true" : "false";
            if (isAutoPlay) StatusLibrary.Log("To disable autoplay: edit eqemupatcher.yml or wait until next patch.");

            IniLibrary.Save();
        }

        private void chkAutoPatch_CheckedChanged(object sender, EventArgs e)
        {
            if (isLoading) return;
            isAutoPatch = chkAutoPatch.Checked;
            IniLibrary.instance.AutoPatch = (isAutoPatch) ? "true" : "false";
            IniLibrary.Save();
        }

        private void MainForm_Shown(object sender, EventArgs e)
        {
            if (isAutoPatch)
            {
                if (isLoading)
                {
                    isPendingPatch = true;
                    pendingPatchTimer.Enabled = true;
                    StatusLibrary.Log("Checking for updates...");
                    SetPrimaryActionChecking();
                    return;
                }

                if (isUpdateAvailable || isNeedingSelfUpdate)
                {
                    StartPatch();
                }
            }
        }

        private string generateSize(double size) {
            if (size < 1024) {
                return $"{Math.Round(size, 2)} bytes";
            }

            size /= 1024;
            if (size < 1024)
            {
                return $"{Math.Round(size, 2)} KB";
            }

            size /= 1024;
            if (size < 1024)
            {
                return $"{Math.Round(size, 2)} MB";
            }

            size /= 1024;
            if (size < 1024)
            {
                return $"{Math.Round(size, 2)} GB";
            }

            return $"{Math.Round(size, 2)} TB";
        }

        private void pendingPatchTimer_Tick(object sender, EventArgs e)
        {
            if (isLoading) return;
            pendingPatchTimer.Enabled = false;
            isPendingPatch = false;
            if (isUpdateAvailable || isNeedingSelfUpdate)
            {
                StartPatch();
                return;
            }

            if (isAutoPlay)
            {
                PlayGame();
            }
        }
    }

    public class FileList
    {
        public string version { get; set; }

        public List<FileEntry> deletes { get; set; }
        public string downloadprefix { get; set; }
        public List<FileEntry> downloads { get; set; }
        public List<FileEntry> unpacks { get; set; }

    }

    public class FileEntry
    {
        public string name { get; set;  }
        public string md5 { get; set; }
        public string date { get; set; }
        public string zip { get; set; }
        public int size { get; set; }
    }

    public class WorkspacePatchersManifest
    {
        public string service { get; set; }
        public string baseUrl { get; set; }
        public string generatedUtc { get; set; }
        public List<WorkspacePatchFeed> feeds { get; set; }
    }

    public class WorkspacePatchFeed
    {
        public string id { get; set; }
        public string label { get; set; }
        public string featureId { get; set; }
        public string clientName { get; set; }
        public string clientPath { get; set; }
        public string patchClient { get; set; }
        public string feedUrl { get; set; }
        public string fileList { get; set; }
        public int downloadCount { get; set; }
        public int missingCount { get; set; }

        public override string ToString()
        {
            return string.IsNullOrWhiteSpace(label) ? id : label;
        }
    }

    public class PatcherServiceStatus
    {
        public string status { get; set; }
        public string message { get; set; }
        public string motd { get; set; }
        public string population { get; set; }
        public string lastUpdated { get; set; }
        public string supportUrl { get; set; }
    }
}
