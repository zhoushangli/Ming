using System;
using System.IO;
using System.Linq;
using Ming;
using SimpleMiner;

internal static class SimpleMinerRuntimeChecks
{
    public static void Ready(WorldScene scene, Camera3D camera)
    {
        input = InputSystem.GetInput();
        Require(ReferenceEquals(input, MingObject.GetGlobalObject<InputSystem>("InputSystem")), "global wrapper identity");
        Require(typeof(InputSystem).GetConstructors().Length == 0, "input has no public constructor");
        Require(!input.IsKeyDown(-1) && !input.WasKeyJustPressed(-1) && !input.WasKeyJustReleased(-1), "normal bound input calls");
        root = MingObject.GetGlobalObject<Ming.FileSystem>("FileSystem").GetResourceRootPath();
        phase = Environment.GetEnvironmentVariable("MING_SIMPLEMINER_PHASE") ?? "first";
        if (phase == "second")
        {
            saved = File.ReadAllLines(Path.Combine(root, "runtime-save.txt")).Select(int.Parse).ToArray();
            camera.SetLocalPosition(new Vector3(saved[0] * 16 + 8, saved[1] * 16 + 8, 150));
        }

        bool missing = false;
        try
        {
            MingObject.GetGlobalObject<InputSystem>("missing");
        }
        catch (InvalidOperationException)
        {
            missing = true;
        }

        Require(missing, "missing global reports an error");
        var noise = new FastNoiseLite(1);
        Require(float.IsFinite(noise.GetNoise(10, 20, 30)), "managed noise is callable");
    }

    public static void Tick(WorldScene scene)
    {
        World world = scene.GetWorld();
        Require(world.LoadsLastFrame <= 1 && world.RebuildsLastFrame <= 4 && world.UnloadsLastFrame <= 4, "frame budgets");
        if (++frames == 1)
        {
            if (phase != "second")
            {
                Chunk chunk = world.Chunks.Values.First();
                int index = Chunk.Index(2, 3, 120);
                byte type = chunk.Blocks[index] == 6 ? (byte)0 : (byte)6;
                Require(world.SetBlock(chunk.Coords.X * 16 + 2, chunk.Coords.Y * 16 + 3, 120, type), "live block edit");
                File.WriteAllLines(Path.Combine(root, "runtime-save.txt"), new[] { chunk.Coords.X, chunk.Coords.Y, (int)type }.Select(value => value.ToString()));
            }
            else
            {
                Require(world.Chunks[new ChunkCoord(saved[0], saved[1])].Blocks[Chunk.Index(2, 3, 120)] == saved[2], "shutdown save survives fresh runtime");
            }
        }

        if (frames == 3)
        {
            using var mesh = new SurfaceTool();
            mesh.SetColor(new Color(12, 34, 56));
            mesh.SetNormal(new Vector3(0, 0, 1));
            mesh.SetUV(new Vector2(0, 0));
            mesh.AddVertex(new Vector3(0, 0, 0));
            mesh.AddVertex(new Vector3(1, 0, 0));
            mesh.AddVertex(new Vector3(0, 1, 0));
            mesh.AddIndex(0);
            mesh.AddIndex(1);
            mesh.AddIndex(2);
            using var resource = mesh.CreateMeshResource();
            testChunk = new Chunk(new ChunkCoord(100, 100), new byte[Chunk.BlockCount]);
            testChunk.Attach(scene);
            testMaterial = new MaterialResource();
        }

        if (frames == 4)
        {
            var definitions = BlockDefinition.Load(Path.Combine(root, "Data/Definitions/BlockSpriteSheet_BlockDefinitions.xml"));
            testChunk.SetBlock(0, 0, 0, 6);
            testChunk.Rebuild(definitions, world.Chunks, testMaterial);
            Require(testChunk.Node.GetMeshResource() != null, "editable chunk creates its first mesh");
            testChunk.SetBlock(0, 0, 0, 0);
            testChunk.Rebuild(definitions, world.Chunks, testMaterial);
            Require(testChunk.Node.GetMeshResource() == null, "removing the final block clears the mesh");
            testChunk.Dispose();
            testMaterial.Dispose();
        }

        if (frames == 8)
        {
            File.WriteAllText(Path.Combine(root, "runtime-" + phase + ".txt"), failed ? "FAIL" : "PASS");
        }
    }

    private static void Require(bool value, string message)
    {
        if (!value)
        {
            failed = true;
            throw new InvalidOperationException("SIMPLEMINER_RUNTIME: " + message);
        }
    }

    private static Chunk testChunk;
    private static MaterialResource testMaterial;
    private static InputSystem input;
    private static string root;
    private static string phase;
    private static int frames;
    private static bool failed;
    private static int[] saved;
}
