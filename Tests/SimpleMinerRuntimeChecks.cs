using System;
using System.IO;
using System.Linq;
using Ming;
using SimpleMiner;
using Math = System.Math;

public partial class SimpleMinerRuntimeChecks : Node
{

    private MinerEngine engine;
    private string root;
    private string phase;
    private int frames;
    private bool failed;

    public override void OnReady()
    {
        root = Ming.FileSystem.GetResourceRootPath();
        phase = Environment.GetEnvironmentVariable("MING_SIMPLEMINER_PHASE") ?? "first";
        engine = g_minerEngine;
        Require(engine != null && engine.Definitions == null, "scene autoload precedes Boot initialization");
        Require(ReferenceEquals(GetNode<MinerEngine>("/Root/MinerEngine"), engine), "absolute node path preserves script identity");
        Require(ReferenceEquals(InputSystem.Singleton, MingObject.GetGlobalObject<InputSystemInstance>("InputSystem")), "global wrapper identity");
        Require(typeof(InputSystem).GetConstructors().Length == 0, "static input has no constructor");
        Require(!InputSystem.IsKeyDown(-1) && !InputSystem.WasKeyJustPressed(-1), "static input invokes native methods");
        Log("SIMPLEMINER_AUTOLOAD_READY");
    }

    public override void OnProcess(float deltaSeconds)
    {
        if (failed)
        {
            return;
        }

        try
        {
            Tick();
        }
        catch
        {
            failed = true;
            File.WriteAllText(Path.Combine(root, "runtime-" + phase + ".txt"), "FAIL");
            throw;
        }
    }

    private void Tick()
    {
        if (GetSceneTree().GetScene() is not WorldScene scene || engine.VoxelDataService.Chunks.Count == 0)
        {
            return;
        }

        VoxelDataService data = engine.VoxelDataService;
        VoxelRenderService render = engine.VoxelRenderService;
        Camera3D camera = scene.GetNode<Camera3D>("Player/Camera");
        Require(ReferenceEquals(engine, g_minerEngine) && GetNodeOrNull("/Root/Boot") == null,
            "Boot was replaced while service tree survived");
        Require(data.LoadsLastFrame <= 1 && render.RebuildsLastFrame <= 4 && data.UnloadsLastFrame <= 4, "frame budgets");
        if (++frames == 1)
        {
            Require(scene.GetNode<Node>(".") == scene && scene.GetNodeOrNull<Camera3D>("VoxelRoot") == null
                && scene.GetNodeOrNull("Missing") == null, "self path and optional missing or mismatched node lookup");
            bool mismatch = false;
            try
            {
                scene.GetNode<Camera3D>("VoxelRoot");
            }
            catch (InvalidCastException)
            {
                mismatch = true;
            }

            Require(mismatch, "required typed lookup rejects wrong type");
            Vector3 original = camera.Position;
            camera.Position = new Vector3(original.X + 1, original.Y, original.Z);
            Require(camera.GetLocalPosition().X == original.X + 1, "position property reaches native transform");
            camera.Position = original;
            EulerAngles rotation = camera.Rotation;
            camera.Rotation = new EulerAngles(rotation.Pitch + 1, rotation.Yaw, rotation.Roll);
            Require(camera.GetLocalOrientation().Pitch == rotation.Pitch + 1, "rotation property preserves degree semantics");
            camera.Rotation = rotation;
            camera.Scale = new Vector3(1, 1, 1);
            Require(camera.GetLocalScale().X == 1 && camera.NearClip == 0.1f && camera.FarClip == 1000f
                && camera.FovDegrees == 60f, "scene camera properties are applied");

            if (phase == "second")
            {
                int[] saved = File.ReadAllLines(Path.Combine(root, "runtime-save.txt")).Select(int.Parse).ToArray();
                Require(data.Chunks[new ChunkCoord(saved[0], saved[1])].Blocks[ChunkData.Index(2, 3, 120)] == saved[2],
                    "shutdown save survives fresh runtime");
            }
            else
            {
                ChunkData chunk = data.Chunks.Values.First();
                byte type = chunk.Blocks[ChunkData.Index(2, 3, 120)] == 6 ? (byte)0 : (byte)6;
                Require(data.SetBlock(chunk.Coords.X * 16 + 2, chunk.Coords.Y * 16 + 3, 120, type), "live block edit");
                File.WriteAllLines(Path.Combine(root, "runtime-save.txt"), new[] { chunk.Coords.X, chunk.Coords.Y, (int)type }.Select(value => value.ToString()));
            }

            VerifyGlobalTransforms();
            VerifyServiceBehaviors();
        }

        if (frames == 8)
        {
            File.WriteAllText(Path.Combine(root, "runtime-" + phase + ".txt"), "PASS");
        }

        if (frames == 3)
        {
            VerifySessionGuards();
        }
    }

    public override void OnExitTree()
    {
        Require(g_minerEngine == null && engine.VoxelDataService.Chunks.Count == 0
            && engine.VoxelRenderService.Nodes.Count == 0, "world and services release before the observer exits");
        File.WriteAllText(Path.Combine(root, "runtime-exit-" + phase + ".txt"), "PASS");
    }

    private void VerifyGlobalTransforms()
    {
        bool Near(Vector3 value, float x, float y, float z) =>
            Math.Abs(value.X - x) < 0.001f && Math.Abs(value.Y - y) < 0.001f && Math.Abs(value.Z - z) < 0.001f;

        var parent = new Node3D();
        var child = new Node3D();
        parent.AddNode(child);
        try
        {
            parent.GlobalPosition = new Vector3(10, 20, 30);
            parent.GlobalRotation = new EulerAngles(0, 90, 0);
            parent.GlobalScale = new Vector3(2, 2, 2);
            child.Position = new Vector3(1, 2, 3);
            Require(Near(child.GlobalPosition, 6, 22, 36), "global position includes parent translation, rotation and scale");
            Require(Math.Abs(child.GlobalRotation.Yaw - 90) < 0.001f && Near(child.GlobalScale, 2, 2, 2),
                "global rotation and scale include the parent transform");

            child.GlobalPosition = new Vector3(14, 26, 38);
            Require(Near(child.Position, 3, -2, 4) && Near(child.GlobalPosition, 14, 26, 38),
                "global position setter converts back to parent space");
            child.GlobalRotation = new EulerAngles(15, 30, 0);
            Require(Math.Abs(child.Rotation.Yaw + 60) < 0.001f && Math.Abs(child.Rotation.Pitch - 15) < 0.001f
                && Math.Abs(child.GlobalRotation.Yaw - 30) < 0.001f && Math.Abs(child.GlobalRotation.Pitch - 15) < 0.001f,
                "global rotation setter preserves degree semantics beneath a rotated parent");
            child.GlobalScale = new Vector3(6, 8, 10);
            Require(Near(child.Scale, 3, 4, 5) && Near(child.GlobalScale, 6, 8, 10),
                "global scale setter accounts for parent scale");
            Require(Near(parent.Position, 10, 20, 30) && Near(parent.GlobalPosition, 10, 20, 30)
                && Math.Abs(parent.Rotation.Yaw - 90) < 0.001f && Near(parent.Scale, 2, 2, 2),
                "global setters equal local setters without a Node3D parent");
        }
        finally
        {
            parent.QueueFree();
        }

        Log("SIMPLEMINER_GLOBAL_TRANSFORMS PASS");
    }

    private void VerifyServiceBehaviors()
    {
        using PackedScene packed = ResourceLoader.Load<PackedScene>("res://Scenes/ServiceChecks.tscn");
        Node services = packed.Instantiate();
        GetRoot().AddNode(services);
        var camera = new Camera3D();
        services.AddNode(camera);
        var view = new Node3D();
        services.AddNode(view);
        var saves = services.GetNode<SaveLoadService>("SaveLoadService");
        var generation = services.GetNode<GenerationService>("GenerationService");
        var data = services.GetNode<VoxelDataService>("VoxelDataService");
        var render = services.GetNode<VoxelRenderService>("VoxelRenderService");
        string fixtures = Path.Combine(root, ".ming", "Verification", "Services", Guid.NewGuid().ToString("N"));
        BlockDefinition[] definitions = engine.Definitions;
        byte stone = BlockDefinition.Find(definitions, "Cobblestone");
        GenerationConfig settings = GenerationConfig.Load(Path.Combine(root, "Data", "GameConfig.xml"));
        generation.Configure(settings, definitions);
        try
        {
            string streaming = Path.Combine(fixtures, "streaming");
            for (int y = -2; y <= 2; y++)
            {
                for (int x = -2; x <= 2; x++)
                {
                    ChunkFile.Write(ChunkFile.GetPath(Path.Combine(streaming, "Saves"), new(x, y)), new byte[ChunkData.BlockCount]);
                }
            }

            saves.Configure(streaming, definitions.Length);
            data.Configure(definitions, saves, generation);
            render.Start(data, definitions, camera, view);
            for (int frame = 0; frame < 9; frame++)
            {
                data.Update(new Vector3(8, 8, 100));
                Require(data.LoadsLastFrame <= 1 && data.UnloadsLastFrame <= 4, "streaming budgets in independent fixture");
            }

            Require(data.Chunks.Count == 9 && data.Chunks.ContainsKey(new(1, 0)), "nearest missing chunks load first");
            render.Rebuild(new(0, 0));
            render.Rebuild(new(1, 0));
            Require(data.SetBlock(15, 2, 20, stone) && !data.SetBlock(15, 2, 20, stone), "identical block writes are no-op");
            Require(render.DirtyChunks.Contains(new(0, 0)) && render.DirtyChunks.Contains(new(1, 0))
                && data.Chunks[new(0, 0)].NeedsSaving && !data.Chunks[new(1, 0)].NeedsSaving,
                "boundary edit invalidates both meshes but saves only changed data");
            render.Rebuild(new(0, 0));
            using (MeshResource mesh = render.Nodes[new(0, 0)].GetMeshResource())
            {
                Require(mesh != null, "first visible block creates a mesh");
            }

            data.SetBlock(15, 2, 20, 0);
            render.Rebuild(new(0, 0));
            Require(render.Nodes[new(0, 0)].GetMeshResource() == null, "final block removal clears the mesh");
            data.SetBlock(15, 2, 20, stone);
            Require(data.SaveChanged(), "changed chunk saves successfully");
            Require(ChunkFile.Read(ChunkFile.GetPath(Path.Combine(streaming, "Saves"), new(0, 0)), definitions.Length)
                .SequenceEqual(data.Chunks[new(0, 0)].Blocks), "full chunk save roundtrip");
            data.Update(new Vector3(900, 900, 100));
            Require(data.UnloadsLastFrame == 4, "distant chunks respect unload budget");
            data.Clear();
            data.Update(new Vector3(8, 8, 100));
            Require(data.Chunks[new(0, 0)].Blocks[ChunkData.Index(15, 2, 20)] == stone, "service data survives close and reopen");
            data.Clear();

            string failedSave = Path.Combine(fixtures, "failed-save");
            Directory.CreateDirectory(failedSave);
            saves.Configure(failedSave, definitions.Length);
            data.Update(new Vector3(8, 8, 100));
            data.SetBlock(3, 3, 120, stone);
            File.WriteAllText(Path.Combine(failedSave, "Saves"), "block directory creation");
            Require(!data.SaveChanged() && data.Chunks[new(0, 0)].NeedsSaving, "save failure retains dirty data");
            data.Update(new Vector3(900, 900, 100));
            Require(data.Chunks.ContainsKey(new(0, 0)), "save failure prevents unload");
            File.Delete(Path.Combine(failedSave, "Saves"));
            Require(data.SaveChanged(), "failed save can be retried");
            data.Clear();

            string damaged = Path.Combine(fixtures, "damaged");
            string path = ChunkFile.GetPath(Path.Combine(damaged, "Saves"), new(0, 0));
            Directory.CreateDirectory(Path.GetDirectoryName(path));
            File.WriteAllBytes(path, new byte[] { 1, 2, 3 });
            saves.Configure(damaged, definitions.Length);
            data.Update(new Vector3(8, 8, 100));
            Require(!data.Chunks.ContainsKey(new(0, 0)) && File.ReadAllBytes(path).SequenceEqual(new byte[] { 1, 2, 3 }),
                "corrupt save is retained and not replaced by generated terrain");
        }
        finally
        {
            render.Stop();
            data.Clear();
            services.QueueFree();
        }

        Log("SIMPLEMINER_SERVICE_BEHAVIORS PASS");
    }

    private void VerifySessionGuards()
    {
        VoxelDataService data = engine.VoxelDataService;
        ChunkData chunk = data.Chunks.Values.First();
        string config = Path.Combine(root, "Data", "GameConfig.xml");
        string original = File.ReadAllText(config);
        try
        {
            File.WriteAllText(config, "<Invalid");
            Require(!engine.ReloadWorld() && ReferenceEquals(data.Chunks[chunk.Coords], chunk),
                "invalid reload configuration preserves the current world");
        }
        finally
        {
            File.WriteAllText(config, original);
        }

        string blocked = Path.Combine(root, ".ming", "Verification", "EndWorld-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(blocked);
        File.WriteAllText(Path.Combine(blocked, "Saves"), "block directory creation");
        engine.SaveLoadService.Configure(blocked, engine.Definitions.Length);
        byte type = chunk.Blocks[ChunkData.Index(3, 4, 125)] == 6 ? (byte)0 : (byte)6;
        data.SetBlock(chunk.Coords.X * 16 + 3, chunk.Coords.Y * 16 + 4, 125, type);
        try
        {
            Require(!engine.EndWorld() && !engine.ReloadWorld() && ReferenceEquals(data.Chunks[chunk.Coords], chunk),
                "save failure blocks explicit world end and reload");
        }
        finally
        {
            engine.SaveLoadService.Configure(root, engine.Definitions.Length);
        }

        Require(engine.ReloadWorld() && data.Chunks.Count == 0 && engine.VoxelRenderService.Nodes.Count == 0
            && ReferenceEquals(engine, g_minerEngine), "successful reload clears session data and presentation while services survive");
        Log("SIMPLEMINER_SESSION_GUARDS PASS");
    }

    private static void Require(bool value, string message)
    {
        if (!value)
        {
            throw new InvalidOperationException("SIMPLEMINER_RUNTIME: " + message);
        }
    }
}
