using EnvDTE;
using EnvDTE80;
using Microsoft.VisualStudio.VCProjectEngine;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Pipes;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;
using System.Text;
using System.Threading;
using System.Xml.Linq;
using System.Linq;

class Program
{
    static class MessagePump
    {
        // user32 imports
        [DllImport("user32.dll")] static extern bool PeekMessage(out MSG lpMsg, IntPtr hWnd, uint wMsgFilterMin, uint wMsgFilterMax, uint wRemoveMsg);
        [DllImport("user32.dll")] static extern bool TranslateMessage(ref MSG lpMsg);
        [DllImport("user32.dll")] static extern IntPtr DispatchMessage(ref MSG lpMsg);
        [DllImport("user32.dll")] static extern uint MsgWaitForMultipleObjectsEx(uint nCount, IntPtr pHandles, uint dwMilliseconds, uint dwWakeMask, uint dwFlags);

        const uint PM_REMOVE = 0x0001;
        const uint QS_ALLINPUT = 0x04FF;
        const uint MWMO_INPUTAVAILABLE = 0x0004;

        [StructLayout(LayoutKind.Sequential)]
        struct MSG { public IntPtr hWnd; public uint message; public IntPtr wParam; public IntPtr lParam; public uint time; public int pt_x; public int pt_y; }

        static void PumpOnce()
        {
            MSG msg;
            while (PeekMessage(out msg, IntPtr.Zero, 0, 0, PM_REMOVE))
            {
                TranslateMessage(ref msg);
                DispatchMessage(ref msg);
            }
        }

        public static void PumpSleep(int milliseconds)
        {
            int end = Environment.TickCount + milliseconds;
            while (unchecked(Environment.TickCount - end) < 0)
            {
                uint remaining = (uint)Math.Max(1, end - Environment.TickCount);
                // メッセージが来るか、タイムアウトするまで待ちつつ、来たら即ポンプ
                MsgWaitForMultipleObjectsEx(0, IntPtr.Zero, remaining, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
                PumpOnce();
            }
        }

        public static void PumpUntil(Func<bool> done, int pollMs = 50, int timeoutMs = 60000)
        {
            var start = Environment.TickCount;
            while (!done())
            {
                PumpSleep(pollMs);
                if (unchecked(Environment.TickCount - start) > timeoutMs)
                    throw new TimeoutException("MessagePump.PumpUntil timeout.");
            }
        }
    }
    static System.Diagnostics.Process FindEditorProcess()
    {
        foreach (var p in System.Diagnostics.Process.GetProcessesByName("ChoEditor"))
        {
            return p; // ChoEditor.exe の最初の1つを返す
        }
        return null;
    }

    // ソリューション内からプロジェクトを名前で探す（ソリューションフォルダ対応）
    static Project FindProjectByName(DTE2 dte, string projName)
    {
        Console.WriteLine(dte.Version);
        foreach (EnvDTE.Project p in dte.Solution.Projects)
        {
            try
            {
                string kind = p.Kind ?? "<null>";
                string full = p.FullName ?? "<null>";
                string name = p.Name ?? "<null>";
                object obj = p.Object;
                string objType = (obj == null) ? "<null>" : obj.GetType().FullName;
                string asmLoc = (obj == null) ? "<null>" : obj.GetType().Assembly.Location;

                Console.WriteLine($"[Proj] Name={name}");
                Console.WriteLine($"       Kind={kind}");
                Console.WriteLine($"       FullName={full}");
                Console.WriteLine($"       ObjectType={objType}");
                Console.WriteLine($"       ObjectAssembly={asmLoc}");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[Proj] Error: {ex.Message}");
            }
        }

        foreach (Project p in dte.Solution.Projects)
        {
            var found = FindProjectRecursive(p, projName);
            if (found != null) return found;
        }
        return null;
    }
    static Project FindProjectRecursive(Project p, string projName)
    {
        if (p == null) return null;
        if (string.Equals(p.Name, projName, StringComparison.OrdinalIgnoreCase))
            return p;

        if (p.Kind == EnvDTE80.ProjectKinds.vsProjectKindSolutionFolder && p.ProjectItems != null)
        {
            foreach (ProjectItem item in p.ProjectItems)
            {
                var sub = item.SubProject;
                var found = FindProjectRecursive(sub, projName);
                if (found != null) return found;
            }
        }
        return null;
    }

    // フィルタを確実に用意
    static VCFilter EnsureFilter(VCProject vcproj, string filterName)
    {
        foreach (VCFilter f in vcproj.Filters)
            if (string.Equals(f.Name, filterName, StringComparison.OrdinalIgnoreCase))
                return f;
        return vcproj.AddFilter(filterName);
    }
    // exe のあるフォルダ
    static string GetExeDir()
    {
        var exePath = System.Reflection.Assembly.GetExecutingAssembly().Location;
        var dir = Path.GetDirectoryName(exePath);
        if (string.IsNullOrEmpty(dir)) dir = AppDomain.CurrentDomain.BaseDirectory;
        return dir;
    }
    static string GetTemplateDir()
    {
        return Path.GetFullPath(Path.Combine(
        GetExeDir(),
        "Cho", "Resources", "EngineAssets", "TemplateScript"));
    }

    // { SCRIPT_NAME } をクラス名に置換
    static string ApplyScriptName(string text, string className)
        => text.Replace("{ SCRIPT_NAME }", className);

    static string GetTemplatePath(string fileName)
    {
        return Path.Combine(GetTemplateDir(), fileName);
    }

    static string LoadTemplateText(string fileName)
    {
        var path = GetTemplatePath(fileName);
        if (!File.Exists(path)) throw new FileNotFoundException("Template not found", path);
        return File.ReadAllText(path, Encoding.UTF8);
    }

    static bool WriteFromTemplateIfEmpty(string absPath, string templFileName, string className)
    {
        var parent = Path.GetDirectoryName(absPath);
        if (string.IsNullOrEmpty(parent))
            throw new ArgumentException("Invalid path: " + absPath);

        Directory.CreateDirectory(parent);

        if (File.Exists(absPath) && new FileInfo(absPath).Length > 0)
            return false;

        var src = LoadTemplateText(templFileName);
        var outText = src.Replace("{ SCRIPT_NAME }", className);

        using (var sw = new StreamWriter(absPath, false, new UTF8Encoding(true)))
            sw.Write(outText);

        return true;
    }
    static string GetProjectDirectory(Project p)
    {
        // p.FullName は .vcxproj のフルパス
        if (!string.IsNullOrEmpty(p.FullName))
        {
            var dir = Path.GetDirectoryName(p.FullName);
            if (!string.IsNullOrEmpty(dir)) return dir;
        }

        // VCProject 側からも試す
        var vcproj = p.Object as Microsoft.VisualStudio.VCProjectEngine.VCProject;
        if (vcproj != null)
        {
            var dir2 = vcproj.ProjectDirectory;
            if (!string.IsNullOrEmpty(dir2))
                return Path.GetFullPath(dir2);
        }

        // 最後の砦（まず来ないはず）
        return AppDomain.CurrentDomain.BaseDirectory;
    }

    static int AddFilesToVcProjectAtDir(
    DTE2 dte, string projectName, string filterName, IEnumerable<string> relativeFilePaths, out string error)
    {
        error = null;
        var p = FindProjectByName(dte, projectName);
        Console.WriteLine(p.FullName);
        Console.WriteLine(p.Kind);
        var vcproj = TryGetVCProject(p);
        if (p == null) { error = $"Project '{projectName}' not found."; return 0; }

        // var vcproj = p.Object as Microsoft.VisualStudio.VCProjectEngine.VCProject;
        if (vcproj == null) { error = $"Project '{projectName}' is not a VC++ project."; return 0; }

        var projDir = GetProjectDirectory(p);

        bool old = dte.SuppressUI;
        dte.SuppressUI = true;

        int added = 0;
        try
        {
            var filter = !string.IsNullOrEmpty(filterName) ? EnsureFilter(vcproj, filterName) : null;

            foreach (var rel in relativeFilePaths)
            {
                if (string.IsNullOrWhiteSpace(rel)) continue;
                var relNorm = rel.Trim().Trim('"').Replace('/', '\\');
                var abs = Path.GetFullPath(Path.Combine(projDir, relNorm));

                try
                {
                    // テンプレ適用（.h/.cpp のみ。必要なら他拡張子も分岐追加）
                    string stem = Path.GetFileNameWithoutExtension(abs);
                    string ext = Path.GetExtension(abs).ToLowerInvariant();
                    switch (ext)
                    {
                        case ".h":
                        case ".hpp":
                        case ".hh":
                            WriteFromTemplateIfEmpty(abs, "TemplateScript.h", stem);
                            break;
                        case ".cpp":
                        case ".cxx":
                            WriteFromTemplateIfEmpty(abs, "TemplateScript.cpp", stem);
                            break;
                        default:
                            var parent = Path.GetDirectoryName(abs);
                            if (string.IsNullOrEmpty(parent))
                                throw new ArgumentException("Invalid path: " + abs);
                            Directory.CreateDirectory(parent);
                            if (!File.Exists(abs))
                                File.WriteAllText(abs, "", new UTF8Encoding(true));
                            break;
                    }

                    if (filter != null) filter.AddFile(abs);
                    else vcproj.AddFile(abs);

                    added++;
                }
                catch (Exception ex)
                {
                    error = (error == null) ? ex.Message : (error + "; " + ex.Message);
                }
            }

            dte.ExecuteCommand("File.SaveAll");
        }
        finally
        {
            dte.SuppressUI = old;
        }
        return added;
    }
    static VCProject TryGetVCProject(EnvDTE.Project p)
    {
        // まず普通のキャスト
        var vc = p.Object as VCProject;
        if (vc != null) return vc;

        // だめなら COM から直接引く
        IntPtr unk = IntPtr.Zero;
        IntPtr ppv = IntPtr.Zero;
        try
        {
            unk = Marshal.GetIUnknownForObject(p.Object);
            var iid = typeof(VCProject).GUID;
            int hr = Marshal.QueryInterface(unk, ref iid, out ppv);
            if (hr == 0 && ppv != IntPtr.Zero)
                return (VCProject)Marshal.GetTypedObjectForIUnknown(ppv, typeof(VCProject));
        }
        finally
        {
            if (ppv != IntPtr.Zero) Marshal.Release(ppv);
            if (unk != IntPtr.Zero) Marshal.Release(unk);
        }
        return null;
    }
    static void AddFilesWithDTE(Project p, IEnumerable<string> absPaths)
    {
        foreach (var abs in absPaths)
            p.ProjectItems.AddFromFile(abs); // ルートに追加
    }
    static Dictionary<string, string> BuildIncludeMapFromVcxproj(string vcxprojPath)
    {
        var map = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        if (!File.Exists(vcxprojPath)) return map;

        XNamespace ns = "http://schemas.microsoft.com/developer/msbuild/2003";
        var doc = XDocument.Load(vcxprojPath);
        var root = doc.Element(ns + "Project");
        if (root == null) return map;

        string projDir = Path.GetDirectoryName(vcxprojPath) ?? "";

        IEnumerable<XElement> items =
            root.Descendants(ns + "ClInclude").Concat(root.Descendants(ns + "ClCompile"));

        foreach (var it in items)
        {
            var inc = (string)it.Attribute("Include");
            if (string.IsNullOrEmpty(inc)) continue;

            // .vcxproj の Include をプロジェクト基準で絶対化
            var abs = Path.GetFullPath(Path.Combine(projDir, inc.Replace('/', '\\')));
            // 絶対パス → .vcxproj上の Include（そのままの文字列）へ
            if (!map.ContainsKey(abs)) map.Add(abs, inc.Replace('/', '\\'));
        }
        return map;
    }
    static void EnsureInFilters_UsingRealInclude(
    string vcxprojPath,
    IEnumerable<(string AbsPath, string Filter)> entries)
    {
        var filtersPath = vcxprojPath + ".filters";
        var doc = File.Exists(filtersPath)
            ? XDocument.Load(filtersPath)
            : new XDocument(new XElement("Project",
                  new XAttribute("ToolsVersion", "4.0"),
                  new XAttribute("xmlns", "http://schemas.microsoft.com/developer/msbuild/2003")));

        XNamespace ns = "http://schemas.microsoft.com/developer/msbuild/2003";
        var root = doc.Element(ns + "Project");
        var includeMap = BuildIncludeMapFromVcxproj(vcxprojPath); // ★ここがポイント

        void EnsureFilterNode(string filterName)
        {
            bool exists = root.Descendants(ns + "Filter")
                              .Any(f => (string)f.Attribute("Include") == filterName);
            if (!exists)
            {
                var ig = new XElement(ns + "ItemGroup");
                ig.Add(new XElement(ns + "Filter",
                          new XAttribute("Include", filterName),
                          new XElement(ns + "UniqueIdentifier", "{" + Guid.NewGuid().ToString().ToUpper() + "}")));
                root.Add(ig);
            }
        }

        foreach (var (abs, filter) in entries)
        {
            // .vcxproj に実際に書かれた Include を使う（見つからなければ自前相対をフォールバック）
            string include = includeMap.TryGetValue(abs, out var inc) ? inc : MakeRelativePath(vcxprojPath, abs);

            string ext = Path.GetExtension(abs).ToLowerInvariant();
            string itemName = (ext == ".h" || ext == ".hpp" || ext == ".hh") ? "ClInclude" : "ClCompile";

            EnsureFilterNode(filter);

            bool already = root.Descendants(ns + itemName)
                               .Any(n => (string)n.Attribute("Include") == include);
            if (already) continue;

            var ig = new XElement(ns + "ItemGroup");
            var item = new XElement(ns + itemName, new XAttribute("Include", include),
                                    new XElement(ns + "Filter", filter));
            ig.Add(item);
            root.Add(ig);
        }

        doc.Save(filtersPath);
    }

    static string MakeRelativePath(string fromFileOrDir, string toPath)
    {
        if (string.IsNullOrEmpty(fromFileOrDir)) return toPath;
        if (string.IsNullOrEmpty(toPath)) return toPath;

        // 基準を「ディレクトリ」に変換
        string baseDir = Directory.Exists(fromFileOrDir)
            ? fromFileOrDir
            : (Path.GetDirectoryName(fromFileOrDir) ?? fromFileOrDir);

        // 末尾に区切りを付けて Uri に食わせる
        string AddSep(string p)
        {
            if (string.IsNullOrEmpty(p)) return p;
            char sep = Path.DirectorySeparatorChar;
            if (!p.EndsWith(sep.ToString()) && !p.EndsWith(Path.AltDirectorySeparatorChar.ToString()))
                p += sep;
            return p;
        }

        try
        {
            var fromUri = new Uri(AddSep(Path.GetFullPath(baseDir)));
            var toUri = new Uri(Path.GetFullPath(toPath));

            // 異なるスキーム（例: file と http）やドライブ違いは相対にできない
            if (!string.Equals(fromUri.Scheme, toUri.Scheme, StringComparison.OrdinalIgnoreCase))
                return toPath;

            var relUri = fromUri.MakeRelativeUri(toUri);
            var rel = Uri.UnescapeDataString(relUri.ToString());

            // Windows 用にスラッシュをバックスラッシュへ
            return rel.Replace('/', Path.DirectorySeparatorChar);
        }
        catch
        {
            // 何かあれば素直に絶対パスを返す
            return toPath;
        }
    }
    // COM一時失敗を自動リトライする小道具
    static void VSInvoke(string label, Action action, int retries = 3, int delayMs = 200)
    {
        for (int i = 0; ; i++)
        {
            try { action(); return; }
            catch (System.Runtime.InteropServices.COMException ex)
            {
                Console.WriteLine($"[COM]{label}: 0x{ex.ErrorCode:X8} {ex.Message}");
                if (i >= retries - 1) throw;
                MessagePump.PumpSleep(delayMs);
            }
        }
    }
    // プロジェクトを選択→Unload→Reload（安全版）
    static void ReloadProjectSilently(EnvDTE80.DTE2 dte, EnvDTE.Project p)
    {
        // 念のため保存＆ビルド/デバッグ待ち
        dte.ExecuteCommand("File.SaveAll");
        if (dte.Solution.SolutionBuild.BuildState != vsBuildState.vsBuildStateDone)
            MessagePump.PumpUntil(() => dte.Solution.SolutionBuild.BuildState != vsBuildState.vsBuildStateInProgress, 50, 60000);
        if (dte.Debugger != null && dte.Debugger.DebuggedProcesses.Count > 0)
            dte.Debugger.Stop(false);

        // SE を前面へ
        var seWin = dte.Windows.Item(EnvDTE.Constants.vsWindowKindSolutionExplorer);
        seWin.Activate();
        var se = (UIHierarchy)seWin.Object;

        // 厳密にプロジェクトノードを選択
        var node = FindProjectNodeByObject(se, p);
        if (node == null)
            throw new InvalidOperationException("Project node not found in Solution Explorer.");

        node.Select(vsUISelectionType.vsUISelectionTypeSelect);
        MessagePump.PumpSleep(100); // 選択反映の猶予

        // コマンドが有効かを確認
        bool canUnload = false;
        try
        {
            var cmd = dte.Commands.Item("Project.UnloadProject");
            canUnload = (cmd != null && cmd.IsAvailable);
        }
        catch { }

        if (!canUnload)
            throw new InvalidOperationException("UnloadProject command is not available (project not selected/unsupported).");

        // 実行
        dte.ExecuteCommand("Project.UnloadProject");
        MessagePump.PumpSleep(300);
        dte.ExecuteCommand("Project.ReloadProject");
        dte.ExecuteCommand("File.SaveAll");
    }

    // ツリー探索（再掲）
    static UIHierarchyItem FindProjectNodeByObject(UIHierarchy se, EnvDTE.Project target)
    {
        foreach (UIHierarchyItem it in se.UIHierarchyItems)
        {
            var hit = FindRec(it, target);
            if (hit != null) return hit;
        }
        return null;

        UIHierarchyItem FindRec(UIHierarchyItem node, EnvDTE.Project tgt)
        {
            try
            {
                // プロジェクト本体ノード？
                var proj = node.Object as EnvDTE.Project;
                if (proj != null)
                {
                    // UniqueName で一致判定（ソリューションフォルダ配下でも一意）
                    if (string.Equals(proj.UniqueName, tgt.UniqueName, StringComparison.OrdinalIgnoreCase))
                        return node;
                }
            }
            catch { /* ignore */ }

            foreach (UIHierarchyItem ch in node.UIHierarchyItems)
            {
                var r = FindRec(ch, tgt);
                if (r != null) return r;
            }
            return null;
        }
    }
    static EnvDTE.UIHierarchyItem FindRecursive(EnvDTE.UIHierarchyItem root, string name)
    {
        if (string.Equals(root.Name, name, StringComparison.OrdinalIgnoreCase))
            return root;
        foreach (EnvDTE.UIHierarchyItem ch in root.UIHierarchyItems)
        {
            var r = FindRecursive(ch, name);
            if (r != null) return r;
        }
        return null;
    }
    static int AddFilesToProject_Fallback(DTE2 dte, string projectName, string filterName, IEnumerable<string> relPaths, out string error)
    {
        error = null;

        var p = FindProjectByName(dte, projectName);
        if (p == null) { error = $"Project '{projectName}' not found."; return 0; }

        var projDir = GetProjectDirectory(p);
        var vcxprojPath = p.FullName; // C:\...\Test.vcxproj

        // 作成＆絶対パス化（テンプレ適用）
        var absList = new List<string>();
        foreach (var rel in relPaths)
        {
            var abs = Path.GetFullPath(Path.Combine(projDir, rel.Trim().Replace('/', '\\')));
            var stem = Path.GetFileNameWithoutExtension(abs);
            var ext = Path.GetExtension(abs).ToLowerInvariant();

            switch (ext)
            {
                case ".h":
                case ".hpp":
                case ".hh": WriteFromTemplateIfEmpty(abs, "TemplateScript.h", stem); break;
                case ".cpp":
                case ".cxx": WriteFromTemplateIfEmpty(abs, "TemplateScript.cpp", stem); break;
                default:
                    Directory.CreateDirectory(Path.GetDirectoryName(abs) ?? ".");
                    if (!File.Exists(abs)) File.WriteAllText(abs, "", new UTF8Encoding(true));
                    break;
            }
            absList.Add(abs);
        }

        // 1) まず DTE でプロジェクトへ追加（ルートに入る）
        AddFilesWithDTE(p, absList);

        // 2) .filters を更新して希望のフィルタへ振り分け
        //    （ヘッダは Header Files に、自動で分岐したいならここで ext みて別Filter名に）
        var pairs = new List<(string, string)>();
        foreach (var abs in absList)
        {
            var ext = Path.GetExtension(abs).ToLowerInvariant();
            string f = string.IsNullOrEmpty(filterName)
                       ? ((ext == ".h" || ext == ".hpp" || ext == ".hh") ? "Header Files" : "Source Files")
                       : filterName;
            pairs.Add((abs, f));
        }
        EnsureInFilters_UsingRealInclude(vcxprojPath, pairs); // ← 新版を使う

        // 3) 保存＋静かにリロード
        dte.ExecuteCommand("File.SaveAll");
        ReloadProjectSilently(dte, p);

        return absList.Count;
    }
    static int ResolveAttachPid(System.Diagnostics.Process parent)
    {
        try
        {
            var name = parent.ProcessName;
            if (name.Equals("devenv", StringComparison.OrdinalIgnoreCase) ||
                name.Equals("msvsmon", StringComparison.OrdinalIgnoreCase))
            {
                var editor = FindEditorProcess();   // 既存の ChoEditor 検索
                if (editor != null) return editor.Id;
            }
        }
        catch { Environment.Exit(0); }
        return parent.Id;
    }
    [STAThread]
    static void Main()
    {
        int parentPid = GetParentProcessId();
        System.Diagnostics.Process parent = null;
        parent = System.Diagnostics.Process.GetProcessById(parentPid);
        int targetPid = ResolveAttachPid(parent);
        parent = System.Diagnostics.Process.GetProcessById(targetPid);
        //try {
        //    parent = System.Diagnostics.Process.GetProcessById(parentPid);
        //    if (parent.ProcessName.Equals("msvsmon", StringComparison.OrdinalIgnoreCase))
        //    {
        //        // 実際には ChoEditor.exe を探して監視する
        //        parent = FindEditorProcess();
        //    }
        //}
        //catch {
        //    Environment.Exit(0); 
        //}
        // Engine が終了したら自分も終了する監視スレッド
        new System.Threading.Thread(() =>
        {
            while (true)
            {
                if (parent.HasExited)
                    Environment.Exit(0);
                MessagePump.PumpSleep(1000);
            }
        })
        { IsBackground = true }.Start();

        // パイプ接続
        // エンジンがイベントを作るまで待つ
        EventWaitHandle waitHandle = null;
        while (true)
        {
            try
            {
                waitHandle = EventWaitHandle.OpenExisting("TheatriaEngineReadyEvent");
                break;
            }
            catch
            {
                MessagePump.PumpSleep(100); // エンジンがまだイベントを作っていない
            }
        }

        // 合図が来るまで待機
        waitHandle.WaitOne();
        Console.WriteLine("合図を受け取りました。パイプ接続します。");

        // パイプ接続
        using (var pipeIn = new NamedPipeClientStream(".", "EngineToWatcher", PipeDirection.In))
        using (var pipeOut = new NamedPipeClientStream(".", "WatcherToEngine", PipeDirection.Out))
        {
            try
            {
                // 両方に接続
                pipeIn.Connect(5000);
                pipeOut.Connect(5000);
                // 5秒タイムアウト
                Console.WriteLine("パイプ接続成功！");
            }
            catch (TimeoutException)
            {
                Console.WriteLine("パイプ接続タイムアウト");
                return;
            }

            Console.WriteLine("Connected to Engine.");

            using (var reader = new StreamReader(pipeIn, Encoding.Unicode))
            using (var writer = new StreamWriter(pipeOut, Encoding.Unicode) { AutoFlush = true })
            {
                while (true)
                {
                    string line = reader.ReadLine();
                    if (line == null)
                    {
                        Console.WriteLine("Engine が終了しました。");
                        break;
                    }

                    Console.WriteLine("[Engineから受信] " + line);

                    switch (line)
                    {
                        // 必ず ACK: で始まる応答を返すこと
                        case "TEST_MESSAGE":
                            writer.WriteLine("ACK:TEST_MESSAGE");
                            break;
                        case "BUILD_START":
                            writer.WriteLine("ACK:BUILD_START");
                            break;
                        case "BUILD_DONE":
                            writer.WriteLine("ACK:BUILD_DONE");
                            break;
                        case string s when s.StartsWith("BUILD_SLN|"):// "BUILD_SLN|Game|Debug|x64"
                            {
                                var parts = s.Split('|');
                                if (parts.Length >= 4)
                                {
                                    string slnName = parts[1];
                                    string config = parts[2];
                                    string platform = parts[3];

                                    // writer.WriteLine("ACK:BUILD_SLN");

                                    bool found = false;
                                    foreach (var dte in GetRunningVisualStudios())
                                    {
                                        try
                                        {
                                            string opened = System.IO.Path.GetFileNameWithoutExtension(dte.Solution.FullName);
                                            if (string.Equals(opened, slnName, StringComparison.OrdinalIgnoreCase))
                                            {
                                                Console.WriteLine($"ビルド対象ソリューション発見: {dte.Solution.FullName}");
                                                found = true;
                                                var configs = dte.Solution.SolutionBuild.SolutionConfigurations;
                                                EnvDTE.SolutionConfiguration targetConfig = null;

                                                foreach (EnvDTE.SolutionConfiguration cfg in configs)
                                                {
                                                    if (string.Equals(cfg.Name, config, StringComparison.OrdinalIgnoreCase))
                                                    {
                                                        // この構成が対象のプラットフォームを持つかチェック
                                                        foreach (EnvDTE.SolutionContext ctx in cfg.SolutionContexts)
                                                        {
                                                            if (string.Equals(ctx.PlatformName, platform, StringComparison.OrdinalIgnoreCase))
                                                            {
                                                                targetConfig = cfg;
                                                                break;
                                                            }
                                                        }
                                                    }
                                                    if (targetConfig != null)
                                                    {
                                                        break;
                                                    }
                                                }
                                                if (targetConfig != null)
                                                {
                                                    targetConfig.Activate();
                                                    dte.Solution.SolutionBuild.Build(true);

                                                    // ビルド完了待ち
                                                    MessagePump.PumpUntil(() => dte.Solution.SolutionBuild.BuildState != vsBuildState.vsBuildStateInProgress, 50, 60000);

                                                    // 判定
                                                    var failedCount = dte.Solution.SolutionBuild.LastBuildInfo;
                                                    bool ok = failedCount == 0;

                                                    Console.WriteLine(ok ? "Build 成功" : $"Build 失敗: {failedCount} 件");
                                                    writer.WriteLine(ok ? "ACK:BUILD_SLN|OK" : $"ACK:BUILD_SLN|FAIL|{failedCount}");
                                                }
                                                else
                                                {
                                                    Console.WriteLine($"構成 {config}|{platform} が見つかりませんでした。");
                                                }
                                                
                                                break;
                                            }
                                        }
                                        catch (Exception ex)
                                        {
                                            Console.WriteLine("DTE 操作中エラー: " + ex.Message);
                                        }
                                    }

                                    if (!found)
                                    {
                                        Console.WriteLine($"指定されたソリューション {slnName} が見つかりません。");
                                    }
                                }
                                break;
                            }
                        case string s when s.StartsWith("BUILD_SLN_DEBUGGER|"):// "BUILD_SLN_DEBUGGER|Game|Debug|x64"
                            {
                                var parts = s.Split('|');
                                if (parts.Length >= 4)
                                {
                                    string slnName = parts[1];
                                    string config = parts[2];
                                    string platform = parts[3];

                                    // writer.WriteLine("ACK:BUILD_SLN");

                                    bool found = false;
                                    foreach (var dte in GetRunningVisualStudios())
                                    {
                                        try
                                        {
                                            string opened = System.IO.Path.GetFileNameWithoutExtension(dte.Solution.FullName);
                                            if (string.Equals(opened, slnName, StringComparison.OrdinalIgnoreCase))
                                            {
                                                Console.WriteLine($"ビルド対象ソリューション発見: {dte.Solution.FullName}");
                                                found = true;
                                                var configs = dte.Solution.SolutionBuild.SolutionConfigurations;
                                                EnvDTE.SolutionConfiguration targetConfig = null;

                                                foreach (EnvDTE.SolutionConfiguration cfg in configs)
                                                {
                                                    if (string.Equals(cfg.Name, config, StringComparison.OrdinalIgnoreCase))
                                                    {
                                                        // この構成が対象のプラットフォームを持つかチェック
                                                        foreach (EnvDTE.SolutionContext ctx in cfg.SolutionContexts)
                                                        {
                                                            if (string.Equals(ctx.PlatformName, platform, StringComparison.OrdinalIgnoreCase))
                                                            {
                                                                targetConfig = cfg;
                                                                break;
                                                            }
                                                        }
                                                    }
                                                    if (targetConfig != null)
                                                    {
                                                        break;
                                                    }
                                                }
                                                if (targetConfig != null)
                                                {
                                                    targetConfig.Activate();
                                                    dte.Solution.SolutionBuild.Build(true);

                                                    // ビルド完了待ち
                                                    MessagePump.PumpUntil(() => dte.Solution.SolutionBuild.BuildState != vsBuildState.vsBuildStateInProgress, 50, 60000);

                                                    // 判定
                                                    var failedCount = dte.Solution.SolutionBuild.LastBuildInfo;
                                                    bool ok = failedCount == 0;

                                                    Console.WriteLine(ok ? "Build 成功" : $"Build 失敗: {failedCount} 件");
                                                    writer.WriteLine(ok ? "ACK:BUILD_SLN|OK" : $"ACK:BUILD_SLN|FAIL|{failedCount}");
                                                    if(!ok) break;

                                                    // 親プロセスIDを取得（すでに持っているなら使う）
                                                    int id = parent.Id;
                                                    Console.WriteLine($"親プロセスID: {id}");

                                                    try
                                                    {
                                                        // VS のデバッガが見ているローカルプロセスを列挙
                                                        foreach (EnvDTE.Process proc in dte.Debugger.LocalProcesses)
                                                        {
                                                            if (proc.ProcessID == id)
                                                            {
                                                                proc.Attach();
                                                                Console.WriteLine("親プロセスにデバッガをアタッチしました。");
                                                                break;
                                                            }
                                                        }
                                                    }
                                                    catch (Exception ex)
                                                    {
                                                        Console.WriteLine("デバッガアタッチ失敗: " + ex.Message);
                                                    }

                                                    writer.WriteLine("ACK:BUILD_SLN|OK");
                                                }
                                                else
                                                {
                                                    Console.WriteLine($"構成 {config}|{platform} が見つかりませんでした。");
                                                }

                                                break;
                                            }
                                        }
                                        catch (Exception ex)
                                        {
                                            Console.WriteLine("DTE 操作中エラー: " + ex.Message);
                                        }
                                    }

                                    if (!found)
                                    {
                                        Console.WriteLine($"指定されたソリューション {slnName} が見つかりません。");
                                    }
                                }
                                break;
                            }
                        case string s when s.StartsWith("STOP_DEBUGGER|"):
                            {
                                var parts = s.Split('|');
                                if (parts.Length >= 2)
                                {
                                    string slnName = parts[1];

                                    foreach (var dte in GetRunningVisualStudios())
                                    {
                                        try
                                        {
                                            string opened = Path.GetFileNameWithoutExtension(dte.Solution.FullName);
                                            if (string.Equals(opened, slnName, StringComparison.OrdinalIgnoreCase))
                                            {
                                                if (dte.Debugger.DebuggedProcesses.Count > 0)
                                                {
                                                    try
                                                    {
                                                        dte.Debugger.Stop(true); // ← Shift+F5 相当
                                                        Console.WriteLine("デバッグを停止しました。");
                                                        writer.WriteLine("ACK:STOP_DEBUGGER");
                                                    }
                                                    catch (Exception ex)
                                                    {
                                                        Console.WriteLine("デバッグ停止失敗: " + ex.Message);
                                                    }
                                                }
                                                else
                                                {
                                                    Console.WriteLine("現在デバッグ中のプロセスはありません。");
                                                    writer.WriteLine("ACK:STOP_DEBUGGER");
                                                }
                                            }
                                        }
                                        catch (Exception ex)
                                        {
                                            Console.WriteLine("デバッガ操作中エラー: " + ex.Message);
                                        }
                                    }
                                }
                                break;
                            }
                        case string s when s.StartsWith("ADD_FILES_PROJ|"):
                            {
                                var parts = s.Split('|');
                                if (parts.Length >= 5)
                                {
                                    string slnName = parts[1];
                                    string projName = parts[2];
                                    string filter = parts[3];
                                    var files = parts[4].Split(new[] { ';' }, StringSplitOptions.RemoveEmptyEntries);

                                    bool slnFound = false;
                                    foreach (var dte in GetRunningVisualStudios())
                                    {
                                        try
                                        {
                                            string opened = Path.GetFileNameWithoutExtension(dte.Solution.FullName);
                                            if (!string.Equals(opened, slnName, StringComparison.OrdinalIgnoreCase))
                                                continue;

                                            slnFound = true;

                                            string error;
                                            int added = AddFilesToVcProjectAtDir(dte, projName, filter, files, out error);
                                            if (string.IsNullOrEmpty(error))
                                                writer.WriteLine($"ACK:ADD_FILES|OK|{added}");
                                            else
                                                writer.WriteLine($"ACK:ADD_FILES|ERROR|{error}");
                                            break;
                                        }
                                        catch (Exception ex)
                                        {
                                            writer.WriteLine($"ACK:ADD_FILES|ERROR|{ex.Message}");
                                            break;
                                        }
                                    }
                                    if (!slnFound)
                                        writer.WriteLine($"ACK:ADD_FILES|ERROR|Solution '{slnName}' not found.");
                                }
                                else writer.WriteLine("ACK:ADD_FILES|ERROR|Bad arguments.");
                                break;
                            }
                        case string s when s.StartsWith("ADD_SCRIPT_PROJ|"):
                            {
                                var parts = s.Split('|');
                                if (parts.Length >= 5)
                                {
                                    string slnName = parts[1];
                                    string projName = parts[2];
                                    string filter = parts[3];    // 例: "Source Files"
                                    string cls = parts[4];    // 例: "Player"

                                    bool slnFound = false;
                                    foreach (var dte in GetRunningVisualStudios())
                                    {
                                        try
                                        {
                                            string opened = Path.GetFileNameWithoutExtension(dte.Solution.FullName);
                                            if (!string.Equals(opened, slnName, StringComparison.OrdinalIgnoreCase))
                                                continue;

                                            slnFound = true;

                                            // ヘッダは Header Files、ソースは指定フィルタへ
                                            string err1, err2;
                                            int n1 = AddFilesToProject_Fallback(dte, projName, "Header Files", new[] { cls + ".h" }, out err1);
                                            int n2 = AddFilesToProject_Fallback(dte, projName, filter, new[] { cls + ".cpp" }, out err2);
                                            //int n1 = AddFilesToVcProjectAtDir(dte, projName, "Header Files", new[] { cls + ".h" }, out err1);
                                            //int n2 = AddFilesToVcProjectAtDir(dte, projName, filter, new[] { cls + ".cpp" }, out err2);

                                            if (string.IsNullOrEmpty(err1) && string.IsNullOrEmpty(err2))
                                                writer.WriteLine($"ACK:ADD_SCRIPT|OK|{n1 + n2}");
                                            else
                                                writer.WriteLine($"ACK:ADD_SCRIPT|ERROR|{(err1 ?? "")} {(err2 ?? "")}");
                                            break;
                                        }
                                        catch (Exception ex)
                                        {
                                            writer.WriteLine($"ACK:ADD_SCRIPT|ERROR|{ex.Message}");
                                            break;
                                        }
                                    }
                                    if (!slnFound)
                                        writer.WriteLine($"ACK:ADD_SCRIPT|ERROR|Solution '{slnName}' not found.");
                                }
                                else writer.WriteLine("ACK:ADD_SCRIPT|ERROR|Bad arguments.");
                                break;
                            }
                        default:
                            writer.WriteLine("ACK:UNKNOWN");
                            break;
                    }
                }
            }
        }

        
    }

    // ROT から DTE を列挙
    static IEnumerable<EnvDTE80.DTE2> GetRunningVisualStudios()
    {
        IRunningObjectTable rot;
        GetRunningObjectTable(0, out rot);
        IEnumMoniker enumMoniker;
        rot.EnumRunning(out enumMoniker);

        IMoniker[] moniker = new IMoniker[1];
        IntPtr fetched = IntPtr.Zero;

        while (enumMoniker.Next(1, moniker, fetched) == 0)
        {
            IBindCtx bindCtx;
            CreateBindCtx(0, out bindCtx);
            string displayName;
            moniker[0].GetDisplayName(bindCtx, null, out displayName);

            if (displayName.StartsWith("!VisualStudio.DTE"))
            {
                object comObject;
                rot.GetObject(moniker[0], out comObject);
                yield return (EnvDTE80.DTE2)comObject;
            }
        }
    }

    [DllImport("ole32.dll")]
    private static extern int CreateBindCtx(uint reserved, out IBindCtx ppbc);

    [DllImport("ole32.dll")]
    private static extern int GetRunningObjectTable(uint reserved, out IRunningObjectTable prot);

    [DllImport("kernel32.dll")]
    static extern IntPtr GetCurrentProcess();

    [DllImport("ntdll.dll")]
    static extern int NtQueryInformationProcess(
        IntPtr processHandle, int processInformationClass,
        ref PROCESS_BASIC_INFORMATION pbi, int processInformationLength, out int returnLength);

    [StructLayout(LayoutKind.Sequential)]
    struct PROCESS_BASIC_INFORMATION
    {
        public IntPtr Reserved1;
        public IntPtr PebBaseAddress;
        public IntPtr Reserved2_0;
        public IntPtr Reserved2_1;
        public IntPtr UniqueProcessId;
        public IntPtr InheritedFromUniqueProcessId; // 親プロセスID
    }

    static int GetParentProcessId()
    {
        PROCESS_BASIC_INFORMATION pbi = new PROCESS_BASIC_INFORMATION();
        NtQueryInformationProcess(GetCurrentProcess(), 0, ref pbi, Marshal.SizeOf(pbi), out _);
        return pbi.InheritedFromUniqueProcessId.ToInt32();
    }
    [ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown),
 Guid("00000016-0000-0000-C000-000000000046")]
    interface IOleMessageFilter
    {
        int HandleInComingCall(
            int dwCallType,
            IntPtr hTaskCaller,
            IntPtr dwTickCount,
            IntPtr lpInterfaceInfo);

        int RetryRejectedCall(
            IntPtr hTaskCallee,
            int dwTickCount,
            int dwRejectType);

        int MessagePending(
            IntPtr hTaskCallee,
            int dwTickCount,
            int dwPendingType);
    }

    static class OLEMessageFilter
    {
        [DllImport("ole32.dll")]
        private static extern int CoRegisterMessageFilter(IOleMessageFilter newFilter, out IOleMessageFilter oldFilter);

        private sealed class MessageFilterImpl : IOleMessageFilter
        {
            public int HandleInComingCall(int dwCallType, IntPtr hTaskCaller, IntPtr dwTickCount, IntPtr lpInterfaceInfo)
                => 0; // SERVERCALL_ISHANDLED

            public int RetryRejectedCall(IntPtr hTaskCallee, int dwTickCount, int dwRejectType)
                => 99; // すぐ再試行（0〜99ms 後）

            public int MessagePending(IntPtr hTaskCallee, int dwTickCount, int dwPendingType)
                => 2;  // PENDINGMSG_WAITDEFPROCESS
        }

        public static void Register()
        {
            IOleMessageFilter _;
            CoRegisterMessageFilter(new MessageFilterImpl(), out _);
        }

        public static void Revoke()
        {
            IOleMessageFilter _;
            CoRegisterMessageFilter(null, out _);
        }
    }
}
