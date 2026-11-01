/* Local visual prototype of the Ming editor. No engine, filesystem or IDE operations are performed. */
(() => {
  'use strict';

  const query = new URLSearchParams(location.search);
  const app = document.querySelector('#app');
  const overlays = document.querySelector('#overlays');
  const ZOOM_STEPS = [25, 50, 75, 100, 150, 200, 300, 400];
  const INVALID_NAME = /[\\/:*?"<>|]/;
  const PREF_DEFAULTS = { scale: '1', tooltips: true, grid: true, snap: false, gridSize: '24', externalIDE: 'Visual Studio', idePath: 'C:/Program Files/Microsoft Visual Studio/', projectName: 'Platformer', renderer: 'Compatibility', viewportW: '640', viewportH: '360' };
  const LAYOUT_DEFAULTS = { left: 220, right: 278, bottom: 132, leftSplit: 392 };

  // Build one scene node with its default transform; `initial` keeps the values Reset restores.
  // e.g. makeNode('pa', 'PlatformA', 'Sprite2D', 'Sprite2D', { x: 60, y: 226, w: 125, h: 18, paint: 'platform' })
  function makeNode(id, name, type, iconName, props = {}, children = [])
  {
    const { w = 0, h = 0, paint = '', owner = '', hasScript = false, ...values } = props;
    const initial = { x: 0, y: 0, rotation: 0, scaleX: 1, scaleY: 1, visible: true, z: 0, filter: 'Nearest', process: 'Inherit', ...values };
    return { id, name, type, icon: iconName, w, h, paint, owner, hasScript, initial, values: { ...initial }, children };
  }

  function makeScene()
  {
    return makeNode('level', 'Level', 'Node2D', 'Node2D', {}, [
      makeNode('env', 'Environment', 'Node2D', 'Node2D', {}, [
        makeNode('ground', 'Ground', 'Sprite2D', 'Sprite2D', { y: 292, w: 640, h: 68, paint: 'ground' }),
        makeNode('platforms', 'Platforms', 'Node2D', 'Node2D', {}, [
          makeNode('pa', 'PlatformA', 'Sprite2D', 'Sprite2D', { x: 60, y: 226, w: 125, h: 18, paint: 'platform' }),
          makeNode('pb', 'PlatformB', 'Sprite2D', 'Sprite2D', { x: 260, y: 174, w: 120, h: 18, paint: 'platform' }),
          makeNode('pc', 'PlatformC', 'Sprite2D', 'Sprite2D', { x: 455, y: 120, w: 125, h: 18, paint: 'platform' })
        ])
      ]),
      makeNode('player', 'Player', 'Node2D', 'Node2D', { x: 276, y: 128, w: 32, h: 46, hasScript: true }, [
        makeNode('sprite', 'Sprite2D', 'Sprite2D', 'Sprite2D', { w: 32, h: 46, paint: 'player', owner: 'player' }),
        makeNode('shape', 'CollisionShape2D', 'CollisionShape2D', 'CollisionShape2D', { x: 3, y: 3, w: 26, h: 40 }),
        makeNode('camera', 'Camera2D', 'Camera2D', 'Camera2D'),
        makeNode('anim', 'AnimationPlayer', 'AnimationPlayer', 'AnimationPlayer')
      ]),
      makeNode('hud', 'HUD', 'Control', 'Control', { x: 16, y: 14 }, [
        makeNode('health', 'HealthBar', 'ProgressBar', 'ProgressBar', { w: 96, h: 8, paint: 'health' })
      ])
    ]);
  }

  const FILES = [
    { name: 'Scenes', icon: 'Folder', children: [{ name: 'Level.ming', icon: 'PackedScene' }, { name: 'Player.ming', icon: 'PackedScene' }] },
    { name: 'Scripts', icon: 'Folder', children: [{ name: 'Player.cs', icon: 'CSharpScript' }] },
    { name: 'Textures', icon: 'Folder', children: [{ name: 'player_idle.png', icon: 'Texture2D' }, { name: 'tileset.png', icon: 'Texture2D' }] }
  ];

  const SETTINGS = {
    Editor: {
      Interface: [{ title: 'Appearance', rows: [['Interface scale', 'scale', 'select', [['1', '100%'], ['1.25', '125%'], ['1.5', '150%']]], ['Show tooltips', 'tooltips', 'check']] }],
      '2D Editor': [{ title: 'Grid & snapping', rows: [['Show grid', 'grid', 'check'], ['Grid size', 'gridSize', 'number', 'px'], ['Enable snapping', 'snap', 'check']] }],
      'External Tools': [{ title: 'External script editor', rows: [['Preferred IDE', 'externalIDE', 'select', [['Visual Studio', 'Visual Studio'], ['Rider', 'Rider'], ['VS Code', 'VS Code']]], ['Executable path', 'idePath', 'text']] }]
    },
    Project: {
      General: [{ title: 'Application', rows: [['Project name', 'projectName', 'text'], ['Renderer', 'renderer', 'select', [['Compatibility', 'Compatibility'], ['Forward', 'Forward']]]] }],
      Display: [{ title: 'Window', rows: [['Viewport width', 'viewportW', 'number', 'px'], ['Viewport height', 'viewportH', 'number', 'px']] }]
    }
  };

  // Node class hierarchy for the Create New Node dialog; a leaf is a plain name.
  // e.g. ['Light2D', ['PointLight2D', 'DirectionalLight2D']] lists two children under Light2D.
  const NODE_CLASSES = ['Node', [
    ['AnimationMixer', ['AnimationPlayer', 'AnimationTree']],
    'AudioStreamPlayer',
    ['CanvasItem', [
      ['Control', [
        ['BaseButton', [['Button', ['CheckBox', 'CheckButton', 'MenuButton', 'OptionButton']], 'LinkButton', 'TextureButton']],
        'ColorRect',
        ['Container', [
          'AspectRatioContainer',
          ['BoxContainer', ['HBoxContainer', ['VBoxContainer', ['ColorPicker']]]],
          'CenterContainer',
          ['FlowContainer', ['HFlowContainer', 'VFlowContainer']],
          'GridContainer', 'MarginContainer', 'PanelContainer', 'ScrollContainer',
          ['SplitContainer', ['HSplitContainer', 'VSplitContainer']],
          'SubViewportContainer', 'TabContainer'
        ]],
        'ItemList', 'Label', 'LineEdit', 'MenuBar', 'NinePatchRect', 'Panel', 'PopupPanel',
        ['Range', ['HScrollBar', 'HSlider', 'ProgressBar', 'SpinBox', 'TextureProgressBar', 'VScrollBar', 'VSlider']],
        'ReferenceRect', 'RichTextLabel',
        ['Separator', ['HSeparator', 'VSeparator']],
        'TabBar',
        ['TextEdit', ['CodeEdit']],
        'TextureRect', 'Tree'
      ]],
      ['Node2D', [
        'AnimatedSprite2D', 'AudioListener2D', 'AudioStreamPlayer2D', 'Bone2D', 'CPUParticles2D', 'Camera2D', 'CanvasGroup', 'CanvasModulate',
        ['CollisionObject2D', ['Area2D', ['PhysicsBody2D', ['CharacterBody2D', 'RigidBody2D', ['StaticBody2D', ['AnimatableBody2D']]]]]],
        'CollisionPolygon2D', 'CollisionShape2D', 'GPUParticles2D',
        ['Joint2D', ['DampedSpringJoint2D', 'GrooveJoint2D', 'PinJoint2D']],
        ['Light2D', ['DirectionalLight2D', 'PointLight2D']],
        'LightOccluder2D', 'Line2D', 'Marker2D', 'MeshInstance2D', 'MultiMeshInstance2D', 'NavigationAgent2D', 'NavigationObstacle2D', 'NavigationRegion2D', 'Parallax2D', 'Path2D', 'PathFollow2D', 'Polygon2D', 'RayCast2D', 'RemoteTransform2D', 'ShapeCast2D', 'Skeleton2D', 'Sprite2D', 'TileMapLayer', 'TouchScreenButton',
        ['VisibleOnScreenNotifier2D', ['VisibleOnScreenEnabler2D']]
      ]]
    ]],
    'CanvasLayer',
    ['Node3D', [
      'AudioListener3D', 'AudioStreamPlayer3D', 'BoneAttachment3D', 'Camera3D',
      ['Collider3D', ['AABBCollider3D', 'CapsuleCollider3D', 'CylinderZCollider3D', 'TriangleMeshCollider3D']],
      ['CollisionObject3D', ['Area3D', ['PhysicsBody3D', ['CharacterBody3D', 'RigidBody3D', ['StaticBody3D', ['AnimatableBody3D']]]]]],
      'CollisionPolygon3D', 'CollisionShape3D', 'Marker3D', 'Path3D', 'PathFollow3D', 'RayCast3D', 'RemoteTransform3D', 'ShapeCast3D', 'Skeleton3D',
      ['VisualInstance3D', [
        ['GeometryInstance3D', ['AnimatedSprite3D', 'GPUParticles3D', 'Label3D', 'MeshInstance3D', 'MultiMeshInstance3D', 'Particles3D', 'Sprite3D']],
        ['Light3D', ['DirectionalLight3D', 'OmniLight3D', 'SpotLight3D']],
        'Skybox3D'
      ]],
      'WorldEnvironment'
    ]],
    'Timer',
    ['Viewport', ['SubViewport', ['Window', [['AcceptDialog', [['ConfirmationDialog', ['FileDialog']]]], 'PopupMenu']]]]
  ]];
  const CLASS_FAVORITES = ['Node2D', 'Node3D', 'Control', 'Sprite2D', 'CharacterBody2D', 'Camera2D'];
  const CLASS_INFO = {
    Node: 'Base of everything in a scene. Holds a name, a parent and children.',
    CanvasItem: 'Abstract base of everything drawn in 2D, including Node2D and Control.',
    Node2D: 'A position, rotation and scale in 2D. Children follow its transform.',
    Node3D: 'A transform in 3D space. Children follow its transform.',
    Control: 'Base of all UI. Lays itself out with anchors and offsets.',
    Sprite2D: 'Draws one texture in 2D.',
    AnimatedSprite2D: 'Plays frame animations from a SpriteFrames resource.',
    Camera2D: 'Chooses which part of the 2D world is shown.',
    Camera3D: 'Chooses which part of the 3D world is shown.',
    CharacterBody2D: 'A body moved from script that slides along what it hits.',
    RigidBody2D: 'A body moved by the physics simulation.',
    StaticBody2D: 'A body that other bodies collide with but that does not move.',
    Area2D: 'Detects bodies and other areas that enter its shape.',
    CollisionShape2D: 'Gives its parent body or area a collision shape.',
    RayCast2D: 'Reports the first thing hit along a line.',
    PointLight2D: 'Lights the 2D scene from a texture at one point.',
    AnimationPlayer: 'Plays animations of any property on any node. Inherits Node, so it can drive 2D, 3D and UI.',
    AudioStreamPlayer: 'Plays sound without a position in the world.',
    AudioStreamPlayer2D: 'Plays sound from a position in 2D.',
    Timer: 'Counts down and signals when it reaches zero.',
    MeshInstance3D: 'Draws one mesh in 3D.',
    OmniLight3D: 'Lights the 3D scene equally in all directions from one point.',
    DirectionalLight3D: 'Lights the whole 3D scene from one direction, like the sun.',
    Container: 'Control that arranges its children for you.',
    HBoxContainer: 'Arranges children in a row.',
    VBoxContainer: 'Arranges children in a column.',
    GridContainer: 'Arranges children in a grid.',
    Button: 'A clickable button with text and an icon.',
    Label: 'Displays text that cannot be edited.',
    LineEdit: 'A single line of editable text.',
    ProgressBar: 'Shows a value as a filled bar.',
    TextureRect: 'Draws a texture inside a UI layout.'
  };

  const S = {
    page: query.get('page') === 'controls' ? 'controls' : 'editor',
    panels: { scene: true, files: true, inspector: true, bottom: true },
    layout: { ...LAYOUT_DEFAULTS },
    inspectorTab: 'Inspector', bottomTab: 'Output', bottomOpen: true, logFilter: 'All messages',
    workspace: '2D', tool: 'select', zoom: 100, pan: { x: 0, y: 0 },
    prefs: { ...PREF_DEFAULTS },
    settingsKind: 'Editor', settingsCategory: 'Interface',
    dialog: null, running: false, paused: false,
    create: { query: '', selected: 'Node2D', folded: new Set(), recent: [] },
    scene: makeScene(), sceneOpen: true, dirty: false,
    selected: 'player', selectedFile: '', rename: null, renameError: '',
    foldedNodes: new Set(), foldedFolders: new Set(), closedGroups: new Set(),
    search: { scene: '', files: '', inspector: '' },
    importValues: { filter: 'Nearest', mipmaps: false, compress: 'Lossless' },
    log: [
      { time: '10:24:01', level: 'info', text: 'Project loaded: Platformer' },
      { time: '10:24:01', level: 'info', text: 'Scene loaded: res://Scenes/Level.ming' },
      { time: '10:24:02', level: 'warn', text: 'tileset.png uses default import settings' }
    ],
    cursor: null, message: '', demo: { filter: 'Nearest', visible: true, tab: 'Output' }
  };

  if (['1', '1.25', '1.5'].includes(query.get('scale')))
  {
    S.prefs.scale = query.get('scale');
  }

  /* Small helpers */
  const esc = value => String(value).replace(/[&<>"']/g, char => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[char]));
  const num = value => (Math.round(Number(value) * 1000) / 1000).toString();
  const clamp = (value, low, high) => Math.max(low, Math.min(high, value));

  let iconCount = 0;

  // Inline one library icon with unique mask ids; a node-icon also takes its class colour.
  // e.g. icon('Sprite2D', 16, 'node-icon') draws the 2D sprite in the Node2D colour.
  function icon(name, size = 16, cls = '')
  {
    const entry = window.MING_ICONS[name];
    if (!entry)
    {
      console.error(`Unknown icon: ${name}`);
      return '';
    }

    iconCount += 1;
    const shape = entry.shape.replace(/"seam"/g, `"seam${iconCount}"`).replace(/#seam\)/g, `#seam${iconCount})`).replace(/"knock"/g, `"knock${iconCount}"`).replace(/#knock\)/g, `#knock${iconCount})`);
    const tone = cls.includes('node-icon') ? `tone-${entry.tone}` : '';
    return `<svg class="icon ${cls} ${tone}" width="${size}" height="${size}" viewBox="0 0 16 16" aria-hidden="true">${shape}</svg>`;
  }

  function button(label, action, cls = '', attributes = '')
  {
    return `<button type="button" class="${cls}" data-action="${esc(action)}" ${attributes}>${label}</button>`;
  }

  function iconButton(name, tip, action, cls = '', attributes = '')
  {
    return button(icon(name), action, `icon-button ${cls}`, `aria-label="${esc(tip)}" data-tip="${esc(tip)}" ${attributes}`);
  }

  // Convert an event position to an element's local pixels, which also holds under interface scale.
  // e.g. localPoint(event, canvas) returns { x: 12, y: 40 } at 125% scale as well as 100%.
  function localPoint(event, element)
  {
    const rect = element.getBoundingClientRect();
    const ratio = rect.width / (element.offsetWidth || 1) || 1;
    return { x: (event.clientX - rect.left) / ratio, y: (event.clientY - rect.top) / ratio };
  }

  function uiScale()
  {
    return Number(S.prefs.scale);
  }

  /* Scene queries */
  function walk(node, visit, parent = null, depth = 0)
  {
    visit(node, parent, depth);
    node.children.forEach(child => walk(child, visit, node, depth + 1));
  }

  function findNode(id)
  {
    let found = null;
    walk(S.scene, node =>
    {
      if (node.id === id)
      {
        found = node;
      }
    });
    return found;
  }

  function parentOf(id)
  {
    let found = null;
    walk(S.scene, (node, parent) =>
    {
      if (node.id === id)
      {
        found = parent;
      }
    });
    return found;
  }

  function selectedNode()
  {
    return S.sceneOpen && !S.selectedFile ? findNode(S.selected) : null;
  }

  function transformOf(node)
  {
    const v = node.values;
    return `translate(${v.x}px, ${v.y}px) rotate(${v.rotation}deg) scale(${v.scaleX}, ${v.scaleY})`;
  }

  function chainTransform(id)
  {
    const chain = [];
    let node = findNode(id);
    while (node)
    {
      chain.unshift(transformOf(node));
      node = parentOf(node.id);
    }
    return chain.join(' ');
  }

  function isModified(node)
  {
    return Object.keys(node.initial).some(key => node.initial[key] !== node.values[key]);
  }

  function logLine(level, text)
  {
    S.log.push({ time: new Date().toTimeString().slice(0, 8), level, text });
  }

  let messageTimer;
  function say(text)
  {
    S.message = text;
    clearTimeout(messageTimer);
    messageTimer = setTimeout(() =>
    {
      S.message = '';
      const element = document.querySelector('.statusbar .message');
      if (element)
      {
        element.textContent = '';
      }
    }, 3200);
    const element = document.querySelector('.statusbar .message');
    if (element)
    {
      element.textContent = text;
    }
  }

  /* Controls */
  const dropdownOptions = {};

  function dropdown(id, value, options, label = '')
  {
    const pairs = options.map(option => Array.isArray(option) ? option : [option, option]);
    dropdownOptions[id] = pairs;
    const shown = pairs.find(pair => String(pair[0]) === String(value));
    return button(`<span class="truncate">${esc(shown ? shown[1] : value)}</span>${icon('ChevronDown', 12)}`, `dropdown:${id}`, 'dropdown', `aria-haspopup="menu" aria-label="${esc(label)}"`);
  }

  function checkbox(checked, action, label, text = '')
  {
    return button(`<span class="box">${checked ? icon('Check', 12) : ''}</span>${esc(text)}`, action, 'check', `role="checkbox" aria-checked="${checked}" aria-label="${esc(label)}"`);
  }

  function search(kind, placeholder)
  {
    return `<div class="search">${icon('Search')}<input type="search" placeholder="${esc(placeholder)}" aria-label="${esc(placeholder)}" data-search="${kind}" data-fid="search-${kind}" value="${esc(S.search[kind])}"></div>`;
  }

  function panelActions(name, key, withClose = true)
  {
    return `<div class="panel-actions">${button(icon('More'), `panel-menu:${key}`, '', `aria-label="${name} panel options" aria-haspopup="menu"`)}${withClose ? button(icon('Close'), `toggle-panel:${key}`, '', `aria-label="Hide ${name} panel"`) : ''}</div>`;
  }

  /* Top bar */
  function topbar()
  {
    const center = S.page === 'editor'
      ? `<nav class="workspaces" aria-label="Workspace">${['2D', '3D'].map(name => button(name, `workspace:${name}`, S.workspace === name ? 'is-active' : '', `aria-pressed="${S.workspace === name}"`)).join('')}</nav>`
      : button('Back to Editor', 'page:editor', 'back-link');
    const running = S.running && !S.paused;
    return `<header class="topbar"><div class="beam" aria-hidden="true"></div><button type="button" class="brand" data-action="about" aria-label="About Ming"><img src="assets/ming-glyph.png" alt=""></button><nav class="menus" aria-label="Editor menu">${['Scene', 'Project', 'Editor', 'View', 'Help'].map(name => button(name, `menu:${name}`, '', 'aria-haspopup="menu" aria-expanded="false"')).join('')}</nav>${center}<div class="run-tools">${iconButton('Play', 'Run project · F5', 'run:play', running ? 'is-running' : '', `aria-pressed="${running}"`)}${iconButton('Pause', 'Pause project', 'run:pause', S.paused ? 'is-running' : '', `aria-pressed="${S.paused}" ${S.running ? '' : 'disabled'}`)}${iconButton('Stop', 'Stop project · F8', 'run:stop', '', S.running ? '' : 'disabled')}<span class="divider"></span>${iconButton('MoreVertical', 'Editor options', 'menu:Editor')}</div></header>`;
  }

  /* Trees */
  // Build the connector from each ancestor down to the selected row, given rows in visible order.
  // e.g. treeGuides([{ id: 'level', depth: 0, leaf: false }, { id: 'player', depth: 1, leaf: true }], 'player') returns a Map of row id to guide markup.
  function treeGuides(rows, selectedId)
  {
    const guides = new Map();
    const add = (id, html) => guides.set(id, (guides.get(id) || '') + html);
    let index = rows.findIndex(row => row.id === selectedId);
    while (index > 0)
    {
      const row = rows[index];
      let parent = index - 1;
      while (parent >= 0 && rows[parent].depth >= row.depth)
      {
        parent -= 1;
      }

      if (parent < 0)
      {
        break;
      }

      // 1) Leave the parent below its fold arrow
      const depth = rows[parent].depth;
      add(rows[parent].id, `<i class="tree-guide is-start" style="--g:${depth}"></i>`);

      // 2) Pass through the rows in between
      for (let between = parent + 1; between < index; between += 1)
      {
        add(rows[between].id, `<i class="tree-guide" style="--g:${depth}"></i>`);
      }

      // 3) Turn into the child and stop short of its arrow, or of its icon on a leaf
      add(row.id, `<i class="tree-guide is-end" style="--g:${depth}"></i><i class="tree-guide is-elbow" style="--g:${depth};--w:${row.leaf ? 18 : 5}px"></i>`);
      index = parent;
    }

    return guides;
  }

  function sceneTreeRows()
  {
    const filter = S.search.scene.toLowerCase();
    const rows = [];
    const matches = node => !filter || node.name.toLowerCase().includes(filter) || node.children.some(matches);
    const visit = (node, depth, hiddenByParent) =>
    {
      if (!matches(node))
      {
        return;
      }

      const selected = S.selected === node.id && !S.selectedFile;
      const hidden = hiddenByParent || !node.values.visible;
      const folded = S.foldedNodes.has(node.id) && !filter;
      const fold = node.children.length
        ? `<span class="tree-fold" data-action="fold-node:${node.id}" aria-label="${folded ? 'Expand' : 'Collapse'} ${esc(node.name)}">${icon(folded ? 'ChevronRight' : 'ChevronDown', 12)}</span>`
        : '<span class="tree-fold"></span>';

      if (S.rename === node.id)
      {
        rows.push({ id: node.id, depth, leaf: !node.children.length, render: guide => `<div class="tree-row is-selected" style="--depth:${depth}">${guide}${fold}${icon(node.icon, 16, 'node-icon')}<input data-fid="rename" data-rename aria-label="Rename ${esc(node.name)}" value="${esc(node.name)}" ${S.renameError ? 'aria-invalid="true"' : ''}></div>` });
      }
      else
      {
        const script = node.hasScript ? `<span class="dim" data-tip="Player.cs attached">${icon('CSharpScript', 12)}</span>` : '';
        const eye = `<span class="tree-eye" data-action="toggle-visible:${node.id}" aria-label="${node.values.visible ? 'Hide' : 'Show'} ${esc(node.name)}">${icon(node.values.visible ? 'Visible' : 'Hidden')}</span>`;
        rows.push({ id: node.id, depth, leaf: !node.children.length, render: guide => button(`${guide}${fold}${icon(node.icon, 16, 'node-icon')}<span class="tree-name truncate">${esc(node.name)}</span>${script}${eye}`, `select-node:${node.id}`, `tree-row ${selected ? 'is-selected' : ''} ${hidden ? 'is-hidden' : ''}`, `style="--depth:${depth}" data-tree="scene" data-id="${node.id}" data-context="node" aria-pressed="${selected}"`) });
      }

      if (!folded)
      {
        node.children.forEach(child => visit(child, depth + 1, hidden));
      }
    };
    visit(S.scene, 0, false);
    const guides = treeGuides(rows, S.selectedFile ? '' : S.selected);
    return rows.map(row => row.render(guides.get(row.id) || '')).join('');
  }

  function sceneTree()
  {
    if (!S.sceneOpen)
    {
      return `<div class="empty">No scene open<br>${button('Open Level.ming', 'open-scene', 'solid')}</div>`;
    }

    const rows = sceneTreeRows();
    return rows ? `<div class="tree" role="group" aria-label="Scene nodes">${rows}</div>` : '<div class="empty">No matching nodes</div>';
  }

  function fileTree()
  {
    const filter = S.search.files.toLowerCase();
    const rows = [];
    FILES.forEach(folder =>
    {
      const children = folder.children.filter(file => !filter || file.name.toLowerCase().includes(filter));
      if (filter && !children.length && !folder.name.toLowerCase().includes(filter))
      {
        return;
      }

      const folded = S.foldedFolders.has(folder.name) && !filter;
      rows.push({ id: `folder:${folder.name}`, depth: 0, leaf: false, render: guide => button(`${guide}<span class="tree-fold" data-action="fold-folder:${folder.name}" aria-label="${folded ? 'Expand' : 'Collapse'} ${folder.name}">${icon(folded ? 'ChevronRight' : 'ChevronDown', 12)}</span>${icon('Folder', 16, 'node-icon')}<span class="tree-name truncate">${folder.name}</span>`, `fold-folder:${folder.name}`, 'tree-row', 'style="--depth:0" data-context="files"') });
      if (!folded)
      {
        children.forEach(file =>
        {
          const selected = S.selectedFile === file.name;
          rows.push({ id: file.name, depth: 1, leaf: true, render: guide => button(`${guide}<span class="tree-fold"></span>${icon(file.icon, 16, 'node-icon')}<span class="tree-name truncate">${file.name}</span>`, `select-file:${file.name}`, `tree-row ${selected ? 'is-selected' : ''}`, `style="--depth:1" data-open="${file.name}" data-context="files" aria-pressed="${selected}"`) });
        });
      }
    });
    const guides = treeGuides(rows, S.selectedFile);
    return rows.length ? `<div class="tree" aria-label="Project resources">${rows.map(row => row.render(guides.get(row.id) || '')).join('')}</div>` : '<div class="empty">No matching files</div>';
  }

  /* Viewport */
  function viewportPanel()
  {
    const toolButton = (name, iconName, tip) => iconButton(iconName, tip, `tool:${name}`, S.tool === name ? 'is-active' : '', `aria-pressed="${S.tool === name}"`);
    const tab = S.sceneOpen
      ? `<div class="scene-tab">${icon('PackedScene', 16, 'scene-icon')}<span>Level.ming${S.dirty ? ' *' : ''}</span>${button(icon('Close', 12), 'close-scene', '', 'aria-label="Close Level.ming"')}</div>`
      : '';
    return `<section class="panel viewport-panel" aria-label="Scene workspace"><div class="panel-header">${tab}${iconButton('Add', 'Open Level.ming', 'open-scene', '', S.sceneOpen ? 'disabled style="height:26px"' : 'style="height:26px"')}${panelActions('Viewport', 'view', false)}</div><div class="canvas-tools">${toolButton('select', 'ToolSelect', 'Select tool · Q')}${toolButton('move', 'ToolMove', 'Move tool · W')}${toolButton('rotate', 'ToolRotate', 'Rotate tool · E')}${toolButton('scale', 'ToolScale', 'Scale tool · R')}<span class="divider"></span>${iconButton('Grid', 'Toggle grid', 'toggle-pref:grid', S.prefs.grid ? 'is-active' : '', `aria-pressed="${S.prefs.grid}"`)}${iconButton('Snap', 'Toggle snapping', 'toggle-pref:snap', S.prefs.snap ? 'is-active' : '', `aria-pressed="${S.prefs.snap}"`)}<span class="readout">Local</span><span class="spacer"></span>${iconButton('ZoomOut', 'Zoom out', 'zoom:-')}<span class="readout" id="zoom-readout">${S.zoom}%</span>${iconButton('ZoomIn', 'Zoom in', 'zoom:+')}${iconButton('CenterView', 'Reset view', 'frame')}</div><div class="canvas-frame"><div class="ruler-corner"></div><div class="ruler ruler-x" id="ruler-x"></div><div class="ruler ruler-y" id="ruler-y"></div><div class="canvas ${S.prefs.grid ? 'show-grid' : ''}" id="canvas"><div class="axis axis-x" id="axis-x"></div><div class="axis axis-y" id="axis-y"></div><div class="stage" id="stage"></div>${canvasMessage()}<div class="viewport-readout"><span id="cursor-readout">X -  Y -</span><span class="spacer"></span><span>Grid ${esc(S.prefs.gridSize)} px</span></div></div></div></section>`;
  }

  function canvasMessage()
  {
    if (S.workspace === '3D')
    {
      return '<div class="canvas-message">3D workspace · layout preview</div>';
    }

    return S.sceneOpen ? '' : `<div class="canvas-message">No scene open${button('Open Level.ming', 'open-scene', 'solid')}</div>`;
  }

  function nodeMarkup(node)
  {
    const paint = node.paint ? `<div class="paint ${node.paint}" data-node="${node.owner || node.id}" style="width:${node.w}px;height:${node.h}px"></div>` : '';
    const v = node.values;
    return `<div class="scene-node" style="transform:${transformOf(node)};z-index:${v.z};visibility:${v.visible ? 'inherit' : 'hidden'}">${paint}${node.children.map(nodeMarkup).join('')}</div>`;
  }

  function selectionMarkup()
  {
    const node = selectedNode();
    if (!node || !node.w || !node.h)
    {
      return '';
    }

    const handles = [[0, 0], [100, 0], [0, 100], [100, 100]].map(([x, y]) => `<i class="handle" style="left:${x}%;top:${y}%"></i>`).join('');
    const gizmo = S.tool === 'move' ? '<div class="gizmo gizmo-x"></div><div class="gizmo gizmo-y"></div>' : '';
    const collider = node.type === 'CollisionShape2D' ? `<div class="collider" style="width:${node.w}px;height:${node.h}px"></div>` : '';
    return `<div class="scene-node" style="transform:${chainTransform(node.id)};z-index:1000">${collider}<div class="selection" style="width:${node.w}px;height:${node.h}px">${handles}<span class="sel-label">${esc(node.name)}</span></div>${gizmo}</div>`;
  }

  // Rebuild the scene objects and selection overlay without touching the panels around them.
  // e.g. a Position X edit calls drawStage() so the inspector keeps its focus and caret.
  function drawStage()
  {
    const stage = document.querySelector('#stage');
    if (!stage)
    {
      return;
    }

    if (S.workspace !== '2D' || !S.sceneOpen)
    {
      stage.innerHTML = '';
      return;
    }

    const frameW = Number(S.prefs.viewportW);
    const frameH = Number(S.prefs.viewportH);
    stage.innerHTML = `<div class="camera-frame" style="width:${frameW}px;height:${frameH}px"><span class="frame-label">Level / Camera2D</span></div>${nodeMarkup(S.scene)}${selectionMarkup()}`;
  }

  function rulerMarkup(axis, length, origin, zoom)
  {
    const major = [24, 48, 96, 192, 384, 768, 1536].find(step => step * zoom >= 72) || 1536;
    const minor = major / 4;
    const start = Math.floor(-origin / zoom / minor) * minor;
    const parts = [];
    for (let world = start; world * zoom + origin <= length; world += minor)
    {
      const at = Math.round(world * zoom + origin);
      const isMajor = world % major === 0;
      const size = isMajor ? 9 : 4;
      parts.push(axis === 'x' ? `<i class="${isMajor ? 'major' : ''}" style="left:${at}px;height:${size}px"></i>` : `<i class="${isMajor ? 'major' : ''}" style="top:${at}px;width:${size}px"></i>`);
      if (isMajor)
      {
        parts.push(axis === 'x' ? `<b style="left:${at + 3}px">${world}</b>` : `<b style="top:${at - 3}px">${world}</b>`);
      }
    }
    parts.push(`<i class="cursor-mark" id="cursor-${axis}" hidden style="${axis === 'x' ? 'height:100%;bottom:0;top:0' : 'width:100%;right:0;left:0'}"></i>`);
    return parts.join('');
  }

  function canvasOrigin(canvas)
  {
    const zoom = S.zoom / 100;
    return {
      x: Math.round((canvas.clientWidth - Number(S.prefs.viewportW) * zoom) / 2 + S.pan.x),
      y: Math.round((canvas.clientHeight - Number(S.prefs.viewportH) * zoom) / 2 + S.pan.y)
    };
  }

  // Apply zoom and pan to the stage, grid, axes and rulers; these are all rects, so the engine can match them.
  // e.g. after a splitter drag the camera frame re-centers and the ruler ticks follow it.
  function syncCanvas()
  {
    const canvas = document.querySelector('#canvas');
    if (!canvas)
    {
      return;
    }

    const zoom = S.zoom / 100;
    const origin = canvasOrigin(canvas);
    const stage = document.querySelector('#stage');
    stage.style.transform = `translate(${origin.x}px, ${origin.y}px) scale(${zoom})`;
    stage.style.setProperty('--z', zoom);

    const gridSize = Number(S.prefs.gridSize) * zoom;
    canvas.style.backgroundSize = `${gridSize}px ${gridSize}px`;
    canvas.style.backgroundPosition = `${origin.x}px ${origin.y}px`;
    document.querySelector('#axis-x').style.top = `${origin.y}px`;
    document.querySelector('#axis-y').style.left = `${origin.x}px`;
    document.querySelector('#ruler-x').innerHTML = rulerMarkup('x', canvas.clientWidth, origin.x, zoom);
    document.querySelector('#ruler-y').innerHTML = rulerMarkup('y', canvas.clientHeight, origin.y, zoom);
    document.querySelector('#zoom-readout').textContent = `${S.zoom}%`;
  }

  function updateCursor(point)
  {
    const readout = document.querySelector('#cursor-readout');
    const canvas = document.querySelector('#canvas');
    if (!readout || !canvas)
    {
      return;
    }

    const markX = document.querySelector('#cursor-x');
    const markY = document.querySelector('#cursor-y');
    if (!point)
    {
      readout.textContent = 'X -  Y -';
      markX.hidden = true;
      markY.hidden = true;
      return;
    }

    const origin = canvasOrigin(canvas);
    const zoom = S.zoom / 100;
    readout.textContent = `X ${Math.round((point.x - origin.x) / zoom)}  Y ${Math.round((point.y - origin.y) / zoom)}`;
    markX.hidden = false;
    markY.hidden = false;
    markX.style.left = `${Math.round(point.x)}px`;
    markX.style.width = '1px';
    markY.style.top = `${Math.round(point.y)}px`;
    markY.style.height = '1px';
  }

  /* Output */
  function outputPanel()
  {
    const errors = S.log.filter(line => line.level === 'error');
    const tabs = ['Output', 'Errors'].map(name =>
    {
      const count = name === 'Errors' ? errors.length : S.log.length;
      return button(`${name}<span class="count">${count}</span>`, `bottom-tab:${name}`, `tab ${S.bottomTab === name ? 'is-selected' : ''}`, `role="tab" aria-selected="${S.bottomTab === name}"`);
    }).join('');
    const toggle = `<div class="panel-actions">${button(icon(S.bottomOpen ? 'ChevronDown' : 'ChevronUp'), 'toggle-bottom', '', `aria-label="${S.bottomOpen ? 'Collapse' : 'Expand'} output" aria-expanded="${S.bottomOpen}"`)}${button(icon('Close'), 'toggle-panel:bottom', '', 'aria-label="Hide Output panel"')}</div>`;
    let body = '';
    if (S.bottomOpen)
    {
      const wanted = { 'All messages': () => true, Info: line => line.level === 'info', Warnings: line => line.level === 'warn' }[S.logFilter];
      const lines = S.bottomTab === 'Errors' ? errors : S.log.filter(wanted);
      const filterControl = S.bottomTab === 'Output' ? `<div style="width:130px">${dropdown('log-filter', S.logFilter, ['All messages', 'Info', 'Warnings'], 'Filter messages')}</div>` : '<span>Errors</span>';
      const rows = lines.map(line => `<div class="log-line"><time>${line.time}</time><span class="log-tag log-${line.level}">${line.level.toUpperCase()}</span><span>${esc(line.text)}</span></div>`).join('');
      body = `<div class="output-tools">${filterControl}<span class="spacer"></span>${button('Clear', 'clear-output', '', 'data-tip="Clear the output log"')}</div><div class="panel-scroll" data-scroll="output">${rows || '<div class="log-line dim">No messages</div>'}</div>`;
    }
    return `<section class="panel bottom-panel ${S.bottomOpen ? '' : 'is-collapsed'}" aria-label="Output panel"><div class="panel-header" role="tablist">${tabs}${toggle}</div>${body}</section>`;
  }

  /* Inspector */
  function numberProperty(node, label, key, unit = '')
  {
    const modified = node.values[key] !== node.initial[key];
    return `<div class="property"><span>${label}</span><span class="field"><input inputmode="decimal" spellcheck="false" aria-label="${label}" data-prop="${key}" data-fid="prop-${key}" value="${num(node.values[key])}"><span>${unit}</span></span>${modified ? button(icon('Undo', 12), `reset-prop:${key}`, 'reset-button', `aria-label="Reset ${label}" data-tip="Reset to ${num(node.initial[key])}"`) : '<span></span>'}</div>`;
  }

  function dropdownProperty(node, label, key, options)
  {
    const modified = node.values[key] !== node.initial[key];
    return `<div class="property"><span>${label}</span>${dropdown(`node-${key}`, node.values[key], options, label)}${modified ? button(icon('Undo', 12), `reset-prop:${key}`, 'reset-button', `aria-label="Reset ${label}" data-tip="Reset to ${node.initial[key]}"`) : '<span></span>'}</div>`;
  }

  function inspectorForFile()
  {
    const name = esc(S.selectedFile);
    const folder = name.endsWith('.cs') ? 'Scripts' : name.endsWith('.png') ? 'Textures' : 'Scenes';
    const kind = name.endsWith('.cs') ? 'C# script' : name.endsWith('.png') ? 'Texture' : 'Scene';
    return `<div class="object-bar">${icon('File')}<span class="truncate">${name}</span></div><div class="class-heading">RESOURCE</div><div class="info-grid"><span>File</span><span>${name}</span><span>Location</span><span>res://${folder}/</span><span>Type</span><span>${kind}</span></div>`;
  }

  function inspectorForNode(node)
  {
    const groups = [
      ['Transform', ['Position X', 'Position Y', 'Rotation', 'Scale X', 'Scale Y'], () => numberProperty(node, 'Position X', 'x', 'px') + numberProperty(node, 'Position Y', 'y', 'px') + numberProperty(node, 'Rotation', 'rotation', 'deg') + numberProperty(node, 'Scale X', 'scaleX') + numberProperty(node, 'Scale Y', 'scaleY')],
      ['Visibility', ['Visible'], () => `<div class="property"><span>Visible</span>${checkbox(node.values.visible, `toggle-visible:${node.id}`, 'Visible', node.values.visible ? 'On' : 'Off')}<span></span></div>`],
      ['Ordering', ['Z Index'], () => numberProperty(node, 'Z Index', 'z')]
    ];
    if (node.type === 'Sprite2D')
    {
      groups.push(['Texture', ['Filter'], () => dropdownProperty(node, 'Filter', 'filter', ['Nearest', 'Linear'])]);
    }

    const filter = S.search.inspector.toLowerCase();
    const shown = groups.filter(([title, labels]) => !filter || title.toLowerCase().includes(filter) || labels.some(label => label.toLowerCase().includes(filter)));
    const sections = shown.map(([title, , content]) =>
    {
      const closed = S.closedGroups.has(title) && !filter;
      return `<section>${button(`${icon(closed ? 'ChevronRight' : 'ChevronDown', 12)}${title}`, `toggle-group:${title}`, 'group-title', `aria-expanded="${!closed}"`)}${closed ? '' : content()}</section>`;
    }).join('');
    const nodeGroup = !filter || 'process mode'.includes(filter) ? `<div class="class-heading">NODE</div>${dropdownProperty(node, 'Process Mode', 'process', ['Inherit', 'Always', 'Disabled'])}` : '';
    return `<div class="object-bar">${icon(node.icon, 16, 'node-icon')}<span class="truncate">${esc(node.name)}</span></div><div class="class-heading">${esc(node.type.toUpperCase())}</div>${sections || (nodeGroup ? '' : '<div class="empty">No matching properties</div>')}${nodeGroup}`;
  }

  function importContent()
  {
    if (!S.selectedFile.endsWith('.png'))
    {
      return '<div class="empty">Select a source texture in FileSystem to adjust its import options.</div>';
    }

    return `<div class="object-bar">${icon('Texture2D', 16, 'node-icon')}<span class="truncate">${esc(S.selectedFile)}</span></div><div class="class-heading">IMPORT AS TEXTURE</div><div class="property"><span>Filter</span>${dropdown('import-filter', S.importValues.filter, ['Nearest', 'Linear'], 'Filter')}<span></span></div><div class="property"><span>Mipmaps</span>${checkbox(S.importValues.mipmaps, 'toggle-import:mipmaps', 'Mipmaps', S.importValues.mipmaps ? 'On' : 'Off')}<span></span></div><div class="property"><span>Compression</span>${dropdown('import-compress', S.importValues.compress, ['Lossless', 'Lossy', 'VRAM'], 'Compression')}<span></span></div><div class="panel-button-row">${button('Reimport', 'reimport', 'solid')}</div>`;
  }

  function inspectorContent()
  {
    if (S.inspectorTab === 'Import')
    {
      return importContent();
    }

    if (S.selectedFile)
    {
      return inspectorForFile();
    }

    const node = selectedNode();
    return node ? inspectorForNode(node) : '<div class="empty">No object selected</div>';
  }

  /* Page assembly */
  function editorPage()
  {
    const { scene, files, inspector, bottom } = S.panels;
    const hasLeft = scene || files;
    const tabs = ['Inspector', 'Import'].map(name => button(name, `inspector-tab:${name}`, `tab ${S.inspectorTab === name ? 'is-selected' : ''}`, `role="tab" aria-selected="${S.inspectorTab === name}"`)).join('');
    const left = `<div class="dock-column left-column">${scene ? `<section class="panel scene-panel" aria-label="Scene panel"><div class="panel-header"><span class="tab is-selected" style="display:inline-flex;align-items:center">Scene</span>${panelActions('Scene', 'scene')}</div><div class="panel-tools">${iconButton('Add', 'Add child node · Ctrl+A', 'add-node', '', S.sceneOpen ? '' : 'disabled')}${search('scene', 'Filter nodes')}</div><div class="panel-scroll" data-scroll="scene">${sceneTree()}</div></section>` : ''}${scene && files ? '<div class="seam seam-y" data-seam="leftSplit"></div>' : ''}${files ? `<section class="panel filesystem-panel" aria-label="FileSystem panel"><div class="panel-header"><span class="tab is-selected" style="display:inline-flex;align-items:center">FileSystem</span>${panelActions('FileSystem', 'files')}</div><div class="path-bar">${iconButton('ChevronLeft', 'Back', 'none', '', 'disabled')}${iconButton('ChevronRight', 'Forward', 'none', '', 'disabled')}<span class="path truncate">res://</span></div><div class="panel-tools">${search('files', 'Filter files')}</div><div class="panel-scroll" data-scroll="files">${fileTree()}</div></section>` : ''}</div>`;
    const center = `<div class="dock-column center-column">${viewportPanel()}${bottom && S.bottomOpen ? '<div class="seam seam-y" data-seam="bottom"></div>' : ''}${bottom ? `${S.bottomOpen ? '' : '<div style="flex:0 0 var(--seam)"></div>'}${outputPanel()}` : ''}</div>`;
    const right = inspector ? `<aside class="panel inspector-panel" aria-label="Inspector panel"><div class="panel-header" role="tablist">${tabs}${panelActions('Inspector', 'inspector')}</div><div class="panel-tools">${search('inspector', 'Filter properties')}</div><div class="panel-scroll" data-scroll="inspector" id="inspector-content">${inspectorContent()}</div></aside>` : '';
    const style = `--left-width:${S.layout.left}px;--right-width:${S.layout.right}px;--bottom-height:${S.layout.bottom}px;--left-split:${S.layout.leftSplit}px`;
    return `<main class="workspace ${hasLeft ? '' : 'no-left'} ${inspector ? '' : 'no-right'}" id="workspace" style="${style}">${hasLeft ? left : '<div></div>'}${hasLeft ? '<div class="seam seam-x" data-seam="left"></div>' : '<div></div>'}${center}${inspector ? '<div class="seam seam-x" data-seam="right"></div>' : '<div></div>'}${right || '<div></div>'}</main>`;
  }

  function controlsPage()
  {
    const row = (label, control) => `<div class="state-row"><span>${label}</span>${control}</div>`;
    const swatches = [['bg', 'var(--bg)'], ['field', 'var(--field)'], ['panel', 'var(--panel)'], ['elevated', 'var(--elevated)'], ['border', 'var(--border)'], ['strong', 'var(--strong)'], ['tint', 'var(--tint)'], ['accent', 'var(--accent)']].map(([name, color]) => `<div class="swatch" style="background:${color};color:${name === 'accent' ? 'var(--bg)' : 'var(--text)'}">${name}</div>`).join('');
    const sampleRows = [['ChevronDown', 'Node2D', 'Level', 0, ''], ['', 'Sprite2D', 'Ground', 1, ''], ['', 'Node2D', 'Player', 1, 'is-selected'], ['', 'Control', 'A_Very_Long_Node_Name_That_Needs_To_Be_Clipped', 1, ''], ['', 'Camera2D', 'HiddenCamera', 1, 'is-hidden']];
    const sampleGuides = treeGuides(sampleRows.map(([fold, , name, depth]) => ({ id: name, depth, leaf: !fold })), 'Player');
    const treeSample = sampleRows.map(([fold, iconName, name, depth, cls]) => `<div class="tree-row ${cls}" style="--depth:${depth};height:24px;align-items:center;display:flex">${sampleGuides.get(name) || ''}<span class="tree-fold">${fold ? icon(fold, 12) : ''}</span>${icon(iconName, 16, 'node-icon')}<span class="tree-name truncate">${name}</span></div>`).join('');
    const library = Object.keys(window.MING_ICONS).map(name => `<span data-tip="${name}">${icon(name, 16, 'node-icon')}</span>`).join('');
    return `<main class="gallery"><div class="gallery-grid">
      <section class="specimen"><h2>Buttons</h2><div class="specimen-body">${row('Normal', button('Button', 'none', 'solid'))}${row('Hover', button('Button', 'none', 'solid state-hover', 'style="background:var(--strong)"'))}${row('Pressed', button('Button', 'none', 'solid state-pressed'))}${row('Focus', button('Button', 'none', 'solid state-focus'))}${row('Selected', button('Button', 'none', 'is-active'))}${row('Disabled', button('Button', 'none', 'solid', 'disabled'))}${row('Primary', button('Save', 'none', 'primary'))}${row('Destructive', button('Delete', 'none', 'solid danger'))}</div></section>
      <section class="specimen"><h2>Fields</h2><div class="specimen-body">${row('Normal', '<input aria-label="Normal sample" value="Player">')}${row('Placeholder', '<input aria-label="Placeholder sample" placeholder="Filter nodes">')}${row('Invalid', '<input aria-label="Invalid sample" aria-invalid="true" value="Player/Body">')}${row('Disabled', '<input aria-label="Disabled sample" disabled style="opacity:.45" value="Read only">')}<div class="property"><span>Numeric</span><span class="field"><input aria-label="Numeric sample" value="276"><span>px</span></span><span></span></div><div class="property"><span>Dropdown</span>${dropdown('demo-filter', S.demo.filter, ['Nearest', 'Linear'], 'Filter sample')}<span></span></div><div class="property"><span>Checkbox</span>${checkbox(S.demo.visible, 'toggle-demo', 'Checkbox sample', S.demo.visible ? 'On' : 'Off')}<span></span></div></div></section>
      <section class="specimen"><h2>Tabs &amp; Tree</h2><div class="specimen-body"><div class="tab-row">${['Output', 'Errors', 'Debugger'].map(name => button(name, `demo-tab:${name}`, `tab ${S.demo.tab === name ? 'is-selected' : ''}`)).join('')}</div><div class="tree" style="padding:0">${treeSample}</div></div></section>
      <section class="specimen"><h2>Notices</h2><div class="specimen-body"><div class="notice">${icon('File')}<div><strong>Changes not saved</strong>Level.ming *</div></div><div class="notice warn">${icon('StatusWarning')}<div><strong>Missing dependency</strong>A texture needs to be reassigned.</div></div><div class="notice failure">${icon('StatusError')}<div><strong>Compile failed</strong>Player.cs:42:17 CS0103</div></div><div class="progress" role="progressbar" aria-valuenow="68" aria-label="Sample progress"><i style="width:68%"></i></div><span class="dim" style="font-size:11px">68% Preparing resources</span></div></section>
      <section class="specimen"><h2>Popups &amp; Dialogs</h2><div class="specimen-body"><div style="display:flex;flex-wrap:wrap;gap:4px">${button('Menu', 'menu:View', 'solid', 'aria-haspopup="menu"')}${button('Tooltip', 'none', 'solid', 'data-tip="A short explanation next to its control."')}${button('Unsaved', 'dialog:unsaved', 'solid')}${button('Delete', 'dialog:delete', 'solid')}${button('Create Node', 'add-node', 'solid')}${button('Settings', 'settings:Editor', 'solid')}${button('About', 'about', 'solid')}</div><div class="sample-scroll" tabindex="0" aria-label="Scrollbar sample"><p>Level / Player</p><p>Level / Camera2D</p><p>Level / Platforms</p><p>Level / HUD</p><p>Level / HealthBar</p><p>Level / Ground</p><p>End of list</p></div></div></section>
      <section class="specimen"><h2>Palette &amp; Type</h2><div class="specimen-body"><div class="swatches">${swatches}</div><div>Body 12 JetBrains Mono</div><div class="muted" style="font-size:11px">Label 11 muted</div><div class="dim" style="font-size:10px">Readout 10 dim 0123456789</div><div style="display:flex;gap:10px;font-size:11px"><span class="log-info">INFO</span><span class="log-warn">WARN</span><span class="log-error">ERROR</span></div></div></section>
      <section class="specimen"><h2>Icon Library</h2><div class="specimen-body"><div class="icon-library">${library}</div></div></section>
    </div></main>`;
  }

  function statusbar()
  {
    const sceneName = S.sceneOpen ? `Level.ming${S.dirty ? ' *' : ''}` : 'No scene';
    const runState = S.running ? `<span class="is-live">${S.paused ? 'PAUSED' : 'RUNNING'}</span>` : '';
    return `<footer class="statusbar"><span>${esc(S.prefs.projectName)}</span><span>/</span><span>${sceneName}</span><span class="message" role="status" aria-live="polite">${esc(S.message)}</span><span class="spacer"></span>${runState}<span>${esc(S.prefs.renderer)}</span><span>${esc(S.prefs.viewportW)} x ${esc(S.prefs.viewportH)}</span></footer>`;
  }

  /* Draw */
  let canvasObserver;
  let isDrawing = false;
  function draw()
  {
    const active = document.activeElement;
    const focusKey = active && active !== document.body ? (active.dataset.fid || active.dataset.action) : '';
    const caret = active && active.tagName === 'INPUT' && active.type !== 'checkbox' ? active.selectionStart : null;
    const scrolls = {};
    document.querySelectorAll('[data-scroll]').forEach(element =>
    {
      scrolls[element.dataset.scroll] = element.scrollTop;
    });

    clearTooltip();
    closeMenu();
    document.documentElement.style.setProperty('--ui-scale', S.prefs.scale);
    document.title = S.page === 'editor' ? `${S.sceneOpen ? `Level.ming${S.dirty ? ' *' : ''}` : 'No scene'} - ${S.prefs.projectName} - Ming` : 'Component Gallery - Ming';
    isDrawing = true;
    app.innerHTML = `<div class="shell">${topbar()}${S.page === 'editor' ? editorPage() : controlsPage()}${statusbar()}</div>`;
    isDrawing = false;

    document.querySelectorAll('[data-scroll]').forEach(element =>
    {
      element.scrollTop = scrolls[element.dataset.scroll] || 0;
    });
    drawStage();
    syncCanvas();
    if (canvasObserver)
    {
      canvasObserver.disconnect();
    }

    const canvas = document.querySelector('#canvas');
    if (canvas)
    {
      canvasObserver = new ResizeObserver(syncCanvas);
      canvasObserver.observe(canvas);
    }

    if (S.rename)
    {
      const input = document.querySelector('[data-rename]');
      if (input && focusKey !== 'rename')
      {
        input.focus();
        input.select();
      }
      else if (input)
      {
        input.focus();
      }
      return;
    }

    if (focusKey)
    {
      const target = document.querySelector(`[data-fid="${CSS.escape(focusKey)}"], [data-action="${CSS.escape(focusKey)}"]`);
      if (target)
      {
        target.focus({ preventScroll: true });
        if (caret !== null && target.setSelectionRange)
        {
          target.setSelectionRange(caret, caret);
        }
      }
    }
  }

  /* Menus */
  let menu = null;

  function closeMenu()
  {
    if (!menu)
    {
      return;
    }

    menu.element.remove();
    menu.trigger.setAttribute('aria-expanded', 'false');
    menu = null;
  }

  function panelMenuItems()
  {
    const rows = [['scene', 'Scene'], ['files', 'FileSystem'], ['inspector', 'Inspector / Import'], ['bottom', 'Output / Errors']];
    return rows.map(([key, label]) => ({ label, action: `toggle-panel:${key}`, check: S.panels[key] }));
  }

  function menuItems(name)
  {
    const node = selectedNode();
    const canDelete = node && node.id !== S.scene.id;
    return ({
      Scene: [{ label: 'New Scene', action: 'close-scene', kbd: 'Ctrl+N' }, { label: 'Open Level.ming', action: 'open-scene', disabled: S.sceneOpen }, null, { label: 'Save Scene', action: 'save', kbd: 'Ctrl+S', disabled: !S.sceneOpen }, { label: 'Close Scene', action: 'close-scene', disabled: !S.sceneOpen }],
      Project: [{ label: 'Project Settings...', action: 'settings:Project' }, { label: 'Open Project Folder', action: 'say:Project folder is not connected in this prototype' }, null, { label: 'Export...', action: 'none', disabled: true }],
      Editor: [{ label: 'Editor Settings...', action: 'settings:Editor' }, { label: 'Restore Default Layout', action: 'reset-layout' }, null, { label: 'Component Gallery', action: 'page:controls' }],
      View: [...panelMenuItems(), null, { label: 'Restore Default Layout', action: 'reset-layout' }],
      Help: [{ label: 'About Ming', action: 'about' }],
      panels: [...panelMenuItems(), null, { label: 'Restore Default Layout', action: 'reset-layout' }],
      node: [{ label: 'Rename', action: 'rename', kbd: 'F2', disabled: !node }, { label: 'Add Child Node...', action: 'add-node', kbd: 'Ctrl+A' }, null, { label: 'Delete', action: 'dialog:delete', kbd: 'Del', disabled: !canDelete }],
      files: [{ label: 'New Folder...', action: 'dialog:new-folder' }, { label: 'Reimport', action: 'reimport', disabled: !S.selectedFile.endsWith('.png') }]
    })[name] || [];
  }

  function openMenu(items, trigger, point = null)
  {
    clearTooltip();
    closeMenu();
    const element = document.createElement('div');
    element.className = 'popup-menu';
    element.setAttribute('role', 'menu');
    element.innerHTML = items.map(item =>
    {
      if (!item)
      {
        return '<hr>';
      }

      return button(`<span class="menu-check">${item.check ? icon('Check', 12) : ''}</span>${esc(item.label)}${item.kbd ? `<kbd>${item.kbd}</kbd>` : ''}`, item.action, '', `role="menuitem" ${item.disabled ? 'disabled' : ''}`);
    }).join('');
    document.body.append(element);

    const scale = uiScale();
    const rect = trigger.getBoundingClientRect();
    const ratio = rect.width / (trigger.offsetWidth || 1) || 1;
    const left = point ? point.x / scale : rect.left / ratio;
    const top = point ? point.y / scale : rect.bottom / ratio + 1;
    const maxLeft = innerWidth / scale - element.offsetWidth - 6;
    const maxTop = innerHeight / scale - element.offsetHeight - 6;
    element.style.left = `${Math.max(4, Math.min(left, maxLeft))}px`;
    element.style.top = `${Math.max(4, Math.min(top, maxTop))}px`;
    trigger.setAttribute('aria-expanded', 'true');
    menu = { element, trigger };
  }

  function openDropdown(id, trigger)
  {
    const current = trigger.querySelector('.truncate').textContent;
    openMenu(dropdownOptions[id].map(([value, label]) => ({ label, action: `pick:${id}:${value}`, check: label === current })), trigger);
    menu.element.style.minWidth = `${trigger.offsetWidth}px`;
    menu.element.querySelector('button')?.focus();
  }

  /* Dialogs */
  function basicDialog(title, content, actions)
  {
    return `<section class="dialog" role="dialog" aria-modal="true" aria-labelledby="dialog-title"><header class="dialog-header"><h2 id="dialog-title">${title}</h2><span class="spacer"></span>${iconButton('Close', 'Close', 'close-dialog')}</header><div class="dialog-body">${content}</div><footer class="dialog-footer">${actions}</footer></section>`;
  }

  function settingsRow(row)
  {
    const [label, key, kind, extra] = row;
    const value = S.prefs[key];
    const modified = value !== PREF_DEFAULTS[key];
    let control = '';
    if (kind === 'check')
    {
      control = checkbox(value, `toggle-pref:${key}`, label, value ? 'On' : 'Off');
    }
    else if (kind === 'select')
    {
      control = dropdown(`pref-${key}`, value, extra, label);
    }
    else
    {
      control = `<span class="field"><input spellcheck="false" aria-label="${label}" data-pref="${key}" data-kind="${kind}" data-fid="pref-${key}" value="${esc(value)}"><span>${kind === 'number' ? extra : ''}</span></span>`;
    }

    return `<div class="property"><span>${label}</span>${control}${modified ? button(icon('Undo', 12), `reset-pref:${key}`, 'reset-button', `aria-label="Reset ${label}" data-tip="Reset to ${esc(PREF_DEFAULTS[key])}"`) : '<span></span>'}</div>`;
  }

  function settingsList(filter = '')
  {
    const needle = filter.toLowerCase();
    const groups = needle
      ? Object.values(SETTINGS[S.settingsKind]).flat()
      : SETTINGS[S.settingsKind][S.settingsCategory];
    const markup = groups.map(group =>
    {
      const rows = group.rows.filter(row => !needle || `${group.title} ${row[0]}`.toLowerCase().includes(needle));
      return rows.length ? `<div class="class-heading">${esc(group.title.toUpperCase())}</div>${rows.map(settingsRow).join('')}` : '';
    }).join('');
    return markup || '<div class="empty">No matching settings</div>';
  }

  function settingsDialog()
  {
    const other = S.settingsKind === 'Editor' ? 'Project' : 'Editor';
    return `<section class="dialog settings-dialog" role="dialog" aria-modal="true" aria-labelledby="dialog-title"><header class="dialog-header"><h2 id="dialog-title">${S.settingsKind} Settings</h2><span class="spacer"></span>${iconButton('Close', 'Close', 'close-dialog')}</header><div class="settings-body"><nav class="settings-nav" aria-label="Settings categories"><div class="nav-heading">${S.settingsKind.toUpperCase()}</div>${Object.keys(SETTINGS[S.settingsKind]).map(name => button(name, `settings-category:${name}`, S.settingsCategory === name ? 'is-selected' : '', `aria-current="${S.settingsCategory === name}"`)).join('')}${button(`Open ${other} Settings`, `settings:${other}`, 'nav-switch')}</nav><div class="settings-main"><div class="panel-tools"><div class="search">${icon('Search')}<input type="search" id="settings-filter" data-fid="settings-filter" placeholder="Search settings" aria-label="Search settings"></div></div><div class="settings-scroll" id="settings-list">${settingsList()}</div></div></div><footer class="dialog-footer">${button('Reset All', 'reset-prefs', 'solid')}${button('Done', 'close-dialog', 'primary')}</footer></section>`;
  }

  /* Create New Node */
  function classParents()
  {
    const parents = {};
    const visit = (entry, parent) =>
    {
      const [name, children] = Array.isArray(entry) ? entry : [entry, []];
      parents[name] = parent;
      children.forEach(child => visit(child, name));
    };
    visit(NODE_CLASSES, '');
    return parents;
  }

  const CLASS_PARENT = classParents();

  function createRows()
  {
    const filter = S.create.query.trim().toLowerCase();
    const rows = [];
    const parts = entry => (Array.isArray(entry) ? entry : [entry, []]);
    const matches = entry =>
    {
      const [name, children] = parts(entry);
      return !filter || name.toLowerCase().includes(filter) || children.some(matches);
    };
    const visit = (entry, depth) =>
    {
      if (!matches(entry))
      {
        return;
      }

      const [name, children] = parts(entry);
      const folded = S.create.folded.has(name) && !filter;
      rows.push({ id: name, depth, leaf: !children.length, folded });
      if (!folded)
      {
        children.forEach(child => visit(child, depth + 1));
      }
    };
    visit(NODE_CLASSES, 0);
    return rows;
  }

  function createList()
  {
    const rows = createRows();
    if (!rows.length)
    {
      return '<div class="empty">No matching node types</div>';
    }

    const guides = treeGuides(rows, S.create.selected);
    const markup = rows.map(row =>
    {
      const selected = S.create.selected === row.id;
      const fold = row.leaf
        ? '<span class="tree-fold"></span>'
        : `<span class="tree-fold" data-action="create-fold:${row.id}" aria-label="${row.folded ? 'Expand' : 'Collapse'} ${row.id}">${icon(row.folded ? 'ChevronRight' : 'ChevronDown', 12)}</span>`;
      const abstract = window.MING_ICONS[row.id].abstract;
      return button(`${guides.get(row.id) || ''}${fold}${icon(row.id, 16, 'node-icon')}<span class="tree-name truncate">${row.id}</span>`, `create-select:${row.id}`, `tree-row ${selected ? 'is-selected' : ''} ${abstract ? 'is-abstract' : ''}`, `style="--depth:${row.depth}" data-create="${row.id}" aria-pressed="${selected}"`);
    }).join('');
    return `<div class="tree" role="group" aria-label="Node types">${markup}</div>`;
  }

  function createNav()
  {
    const item = name => button(`${icon(name, 16, 'node-icon')}<span class="truncate">${name}</span>`, `create-select:${name}`, S.create.selected === name ? 'is-selected' : '');
    const recent = S.create.recent.length ? S.create.recent.map(item).join('') : '<div class="nav-empty">Nothing yet</div>';
    return `<div class="nav-heading">FAVORITES</div>${CLASS_FAVORITES.map(item).join('')}<div class="nav-heading">RECENT</div>${recent}`;
  }

  function createInfo()
  {
    const name = S.create.selected;
    const parent = CLASS_PARENT[name];
    const abstract = window.MING_ICONS[name].abstract;
    const lineage = `${abstract ? 'Abstract class. ' : ''}${parent ? `Inherits ${parent}.` : 'Root of all nodes.'}`;
    return `<strong>${name}</strong><span class="dim">${lineage}</span><span>${CLASS_INFO[name] || ''}</span>`;
  }

  function createDialog()
  {
    const parent = selectedNode() || S.scene;
    return `<section class="dialog create-dialog" role="dialog" aria-modal="true" aria-labelledby="dialog-title"><header class="dialog-header"><h2 id="dialog-title">Create New Node</h2><span class="spacer"></span>${iconButton('Close', 'Close', 'close-dialog')}</header><div class="create-body"><nav class="settings-nav" id="create-nav" aria-label="Favorite and recent node types">${createNav()}</nav><div class="settings-main"><div class="panel-tools"><div class="search">${icon('Search')}<input type="search" id="create-search" placeholder="Search node types" aria-label="Search node types" autocomplete="off" value="${esc(S.create.query)}"></div></div><div class="settings-scroll" id="create-list">${createList()}</div><div class="create-info" id="create-info">${createInfo()}</div></div></div><footer class="dialog-footer"><span class="spacer truncate dim create-parent">Child of ${esc(parent.name)}</span>${button('Cancel', 'close-dialog', 'solid')}${button('Create', 'create-confirm', 'primary', `id="create-confirm" ${window.MING_ICONS[S.create.selected].abstract ? 'disabled' : ''}`)}</footer></section>`;
  }

  // Redraw only the parts of the dialog that depend on the selection, so the search box keeps focus.
  // e.g. pressing ArrowDown in the search box moves the highlight and updates the description.
  function refreshCreate(reveal = false)
  {
    document.querySelector('#create-nav').innerHTML = createNav();
    document.querySelector('#create-list').innerHTML = createList();
    document.querySelector('#create-info').innerHTML = createInfo();
    document.querySelector('#create-confirm').disabled = window.MING_ICONS[S.create.selected].abstract;
    if (reveal)
    {
      document.querySelector('#create-list .is-selected')?.scrollIntoView({ block: 'nearest' });
    }
  }

  function moveCreateSelection(step)
  {
    const rows = createRows();
    const index = rows.findIndex(row => row.id === S.create.selected);
    const next = rows[clamp(index + step, 0, rows.length - 1)];
    if (next)
    {
      S.create.selected = next.id;
      refreshCreate(true);
    }
  }

  // Keep the current type while it still matches; otherwise pick the best concrete match.
  // e.g. typing "sprite" selects Sprite2D rather than the abstract classes above it.
  function filterCreate(query)
  {
    S.create.query = query;
    const needle = query.trim().toLowerCase();
    const named = createRows().filter(row => row.id.toLowerCase().includes(needle) && !window.MING_ICONS[row.id].abstract);
    if (needle && named.length && !S.create.selected.toLowerCase().includes(needle))
    {
      const best = named.find(row => row.id.toLowerCase().startsWith(needle)) || named[0];
      S.create.selected = best.id;
    }

    refreshCreate(true);
  }

  function openCreateNode()
  {
    S.create.query = '';
    openDialog('create-node');
    document.querySelector('#create-list .is-selected')?.scrollIntoView({ block: 'center' });
  }

  function confirmCreate()
  {
    const type = S.create.selected;
    if (window.MING_ICONS[type].abstract)
    {
      return;
    }

    S.create.recent = [type, ...S.create.recent.filter(name => name !== type)].slice(0, 6);
    closeDialog();
    addChildNode(type);
    say(`Created ${type}`);
    document.querySelector('[data-tree="scene"].is-selected')?.focus();
  }

  function dialogMarkup()
  {
    switch (S.dialog)
    {
      case 'create-node':
        return createDialog();
      case 'settings':
        return settingsDialog();
      case 'unsaved':
        return basicDialog('Save changes before closing?', '<p><strong>Level.ming</strong> has unsaved changes.</p>', button('Cancel', 'close-dialog', 'solid') + button('Discard', 'discard-scene', 'solid') + button('Save', 'save-and-close', 'primary'));
      case 'delete':
      {
        const node = selectedNode();
        return basicDialog('Delete node?', `<p>This removes <strong>${esc(node ? node.name : 'the selected node')}</strong> and its children from the scene.</p>`, button('Cancel', 'close-dialog', 'solid') + button('Delete', 'delete-node', 'solid danger'));
      }
      case 'new-folder':
        return basicDialog('New folder', '<label for="folder-name">Folder name</label><input id="folder-name" data-fid="folder-name" data-folder-name value="NewFolder"><div class="form-error" id="folder-error" role="status"></div>', button('Cancel', 'close-dialog', 'solid') + button('Create', 'create-folder', 'primary', 'id="folder-create"'));
      default:
        return basicDialog('About Ming', '<p><strong>Ming Editor</strong> visual prototype, Warm Carbon Ink.</p><p>Everything here runs in the browser and stays in memory until reload. No engine, IDE, filesystem, compiler or export process is connected.</p>', button('Close', 'close-dialog', 'primary'));
    }
  }

  let returnFocus = null;

  function renderDialog()
  {
    overlays.innerHTML = `<div class="modal-backdrop">${dialogMarkup()}</div>`;
    app.inert = true;
  }

  function openDialog(kind)
  {
    clearTooltip();
    closeMenu();
    returnFocus = document.activeElement;
    S.dialog = kind;
    renderDialog();
    const first = overlays.querySelector('.dialog-body input, .settings-main input, .dialog-footer .primary, .dialog-footer button');
    first?.focus();
  }

  function closeDialog()
  {
    S.dialog = null;
    overlays.innerHTML = '';
    app.inert = false;
    if (returnFocus && returnFocus.isConnected)
    {
      returnFocus.focus();
    }
  }

  /* Edits */
  function markDirty()
  {
    if (!S.dirty)
    {
      S.dirty = true;
      return true;
    }

    return false;
  }

  function setPref(key, value)
  {
    S.prefs[key] = value;
    draw();
    if (S.dialog)
    {
      const filter = document.querySelector('#settings-filter');
      const list = document.querySelector('#settings-list');
      if (list)
      {
        list.innerHTML = settingsList(filter ? filter.value : '');
      }
    }
  }

  function save()
  {
    S.dirty = false;
    logLine('info', 'Scene saved: res://Scenes/Level.ming');
    say('Saved Level.ming');
    draw();
  }

  function closeScene()
  {
    S.sceneOpen = false;
    S.selected = '';
    S.dirty = false;
    S.scene = makeScene();
    S.foldedNodes.clear();
    draw();
  }

  function openScene()
  {
    if (S.sceneOpen)
    {
      return;
    }

    S.sceneOpen = true;
    S.selected = 'player';
    S.selectedFile = '';
    logLine('info', 'Scene loaded: res://Scenes/Level.ming');
    draw();
  }

  function beginRename()
  {
    const node = selectedNode();
    if (node)
    {
      S.rename = node.id;
      S.renameError = '';
      draw();
    }
  }

  function commitRename(value)
  {
    const node = findNode(S.rename);
    const name = value.trim();
    if (!node)
    {
      S.rename = null;
      return;
    }

    if (!name || INVALID_NAME.test(name))
    {
      S.renameError = 'invalid';
      say('Names cannot be empty or use / \\ : * ? " < > |');
      draw();
      return;
    }

    if (name !== node.name)
    {
      node.name = name;
      markDirty();
    }
    S.rename = null;
    S.renameError = '';
    draw();
    document.querySelector(`[data-tree="scene"][data-id="${node.id}"]`)?.focus();
  }

  function cancelRename()
  {
    const id = S.rename;
    S.rename = null;
    S.renameError = '';
    draw();
    document.querySelector(`[data-tree="scene"][data-id="${id}"]`)?.focus();
  }

  function addChildNode(type)
  {
    const parent = selectedNode() || S.scene;
    let name = type;
    let index = 1;
    let exists = true;
    while (exists)
    {
      const candidate = name;
      exists = false;
      walk(S.scene, node =>
      {
        exists = exists || node.name === candidate;
      });
      if (exists)
      {
        index += 1;
        name = `${type}${index}`;
      }
    }

    const node = makeNode(`node-${Date.now()}`, name, type, type);
    parent.children.push(node);
    S.foldedNodes.delete(parent.id);
    S.selected = node.id;
    S.selectedFile = '';
    markDirty();
    draw();
  }

  function deleteNode()
  {
    const node = selectedNode();
    const parent = node ? parentOf(node.id) : null;
    if (parent)
    {
      parent.children = parent.children.filter(child => child !== node);
      S.selected = parent.id;
      markDirty();
    }
    closeDialog();
    draw();
  }

  function stepZoom(direction, anchor = null)
  {
    const canvas = document.querySelector('#canvas');
    const index = ZOOM_STEPS.indexOf(S.zoom);
    const next = ZOOM_STEPS[clamp(index + direction, 0, ZOOM_STEPS.length - 1)];
    if (next === S.zoom || !canvas)
    {
      return;
    }

    const pivot = anchor || { x: canvas.clientWidth / 2, y: canvas.clientHeight / 2 };
    const origin = canvasOrigin(canvas);
    const worldX = (pivot.x - origin.x) / (S.zoom / 100);
    const worldY = (pivot.y - origin.y) / (S.zoom / 100);
    S.zoom = next;
    const moved = canvasOrigin(canvas);
    S.pan.x += pivot.x - (moved.x + worldX * next / 100);
    S.pan.y += pivot.y - (moved.y + worldY * next / 100);
    syncCanvas();
    say(`Zoom ${next}%`);
  }

  function moveTreeSelection(direction)
  {
    const rows = [...document.querySelectorAll('[data-tree="scene"]')];
    const index = rows.findIndex(row => row.dataset.id === S.selected);
    const next = rows[clamp(index + direction, 0, rows.length - 1)];
    if (next)
    {
      S.selected = next.dataset.id;
      S.selectedFile = '';
      draw();
      document.querySelector(`[data-tree="scene"][data-id="${S.selected}"]`)?.focus();
    }
  }

  function pick(id, value)
  {
    if (id.startsWith('pref-'))
    {
      setPref(id.slice(5), value);
    }
    else if (id.startsWith('node-'))
    {
      const node = selectedNode();
      if (node)
      {
        node.values[id.slice(5)] = value;
        markDirty();
      }
      draw();
    }
    else if (id.startsWith('import-'))
    {
      S.importValues[id.slice(7)] = value;
      draw();
    }
    else if (id === 'log-filter')
    {
      S.logFilter = value;
      draw();
    }
    else if (id === 'demo-filter')
    {
      S.demo.filter = value;
      draw();
    }
  }

  /* Actions */
  const actions = {
    'none': () => {},
    'about': () => openDialog('about'),
    'say': value => say(value),
    'dialog': value => openDialog(value),
    'close-dialog': () => closeDialog(),
    'page': value =>
    {
      S.page = value;
      draw();
    },
    'menu': (value, target) =>
    {
      if (menu && menu.trigger === target)
      {
        closeMenu();
        return;
      }

      openMenu(menuItems(value), target);
    },
    'panel-menu': (value, target) =>
    {
      if (menu && menu.trigger === target)
      {
        closeMenu();
        return;
      }

      openMenu(menuItems('panels'), target);
    },
    'dropdown': (value, target) =>
    {
      if (menu && menu.trigger === target)
      {
        closeMenu();
        return;
      }

      openDropdown(value, target);
    },
    'pick': value =>
    {
      const split = value.indexOf(':');
      pick(value.slice(0, split), value.slice(split + 1));
    },
    'workspace': value =>
    {
      S.workspace = value;
      draw();
    },
    'tool': value =>
    {
      S.tool = value;
      draw();
    },
    'run': value =>
    {
      if (value === 'play')
      {
        if (!S.running)
        {
          logLine('info', 'Running project');
        }
        S.running = true;
        S.paused = false;
      }
      else if (value === 'pause')
      {
        S.paused = !S.paused;
      }
      else
      {
        S.running = false;
        S.paused = false;
        logLine('info', 'Project stopped');
      }
      draw();
    },
    'select-node': value =>
    {
      S.selected = value;
      S.selectedFile = '';
      if (S.inspectorTab === 'Import')
      {
        S.inspectorTab = 'Inspector';
      }
      draw();
    },
    'fold-node': value =>
    {
      if (S.foldedNodes.has(value))
      {
        S.foldedNodes.delete(value);
      }
      else
      {
        S.foldedNodes.add(value);
      }
      draw();
    },
    'toggle-visible': value =>
    {
      const node = findNode(value);
      node.values.visible = !node.values.visible;
      markDirty();
      draw();
    },
    'select-file': value =>
    {
      S.selectedFile = value;
      draw();
    },
    'fold-folder': value =>
    {
      if (S.foldedFolders.has(value))
      {
        S.foldedFolders.delete(value);
      }
      else
      {
        S.foldedFolders.add(value);
      }
      draw();
    },
    'toggle-group': value =>
    {
      if (S.closedGroups.has(value))
      {
        S.closedGroups.delete(value);
      }
      else
      {
        S.closedGroups.add(value);
      }
      draw();
    },
    'reset-prop': value =>
    {
      const node = selectedNode();
      node.values[value] = node.initial[value];
      draw();
    },
    'inspector-tab': value =>
    {
      S.inspectorTab = value;
      draw();
    },
    'bottom-tab': value =>
    {
      S.bottomTab = value;
      S.bottomOpen = true;
      draw();
    },
    'toggle-bottom': () =>
    {
      S.bottomOpen = !S.bottomOpen;
      draw();
    },
    'clear-output': () =>
    {
      S.log = [];
      draw();
    },
    'toggle-panel': value =>
    {
      S.panels[value] = !S.panels[value];
      draw();
    },
    'reset-layout': () =>
    {
      S.panels = { scene: true, files: true, inspector: true, bottom: true };
      S.layout = { ...LAYOUT_DEFAULTS };
      S.inspectorTab = 'Inspector';
      S.bottomOpen = true;
      S.search = { scene: '', files: '', inspector: '' };
      say('Default layout restored');
      draw();
    },
    'zoom': value => stepZoom(value === '+' ? 1 : -1),
    'frame': () =>
    {
      S.zoom = 100;
      S.pan = { x: 0, y: 0 };
      syncCanvas();
    },
    'toggle-pref': value => setPref(value, !S.prefs[value]),
    'reset-pref': value => setPref(value, PREF_DEFAULTS[value]),
    'reset-prefs': () =>
    {
      S.prefs = { ...PREF_DEFAULTS };
      draw();
      renderDialog();
    },
    'settings': value =>
    {
      S.settingsKind = value;
      S.settingsCategory = Object.keys(SETTINGS[value])[0];
      if (S.dialog === 'settings')
      {
        renderDialog();
      }
      else
      {
        openDialog('settings');
      }
    },
    'settings-category': value =>
    {
      S.settingsCategory = value;
      renderDialog();
    },
    'save': () => save(),
    'close-scene': () =>
    {
      if (S.dirty)
      {
        openDialog('unsaved');
        return;
      }

      closeScene();
    },
    'open-scene': () => openScene(),
    'discard-scene': () =>
    {
      closeDialog();
      closeScene();
    },
    'save-and-close': () =>
    {
      closeDialog();
      save();
      closeScene();
    },
    'rename': () => beginRename(),
    'add-node': () => openCreateNode(),
    'create-select': value =>
    {
      if (S.create.selected !== value)
      {
        S.create.selected = value;
        refreshCreate();
      }
      document.querySelector('#create-search').focus();
    },
    'create-fold': value =>
    {
      if (!S.create.folded.delete(value))
      {
        S.create.folded.add(value);
      }
      refreshCreate();
      document.querySelector('#create-search').focus();
    },
    'create-confirm': () => confirmCreate(),
    'delete-node': () => deleteNode(),
    'reimport': () =>
    {
      logLine('info', `Reimported ${S.selectedFile}`);
      say(`Reimported ${S.selectedFile}`);
      draw();
    },
    'toggle-import': value =>
    {
      S.importValues[value] = !S.importValues[value];
      draw();
    },
    'create-folder': () =>
    {
      const input = document.querySelector('#folder-name');
      if (validateFolder(input))
      {
        const name = input.value.trim();
        closeDialog();
        say(`Folder ${name} accepted, prototype only`);
      }
    },
    'toggle-demo': () =>
    {
      S.demo.visible = !S.demo.visible;
      draw();
    },
    'demo-tab': value =>
    {
      S.demo.tab = value;
      draw();
    }
  };

  function validateFolder(input)
  {
    const name = input.value.trim();
    const valid = name.length > 0 && !INVALID_NAME.test(name);
    input.setAttribute('aria-invalid', String(!valid));
    document.querySelector('#folder-error').textContent = valid ? '' : 'Enter a name without reserved characters.';
    document.querySelector('#folder-create').disabled = !valid;
    return valid;
  }

  /* Events */
  document.addEventListener('click', event =>
  {
    const target = event.target.closest('[data-action]');
    if (target && !target.disabled)
    {
      const [name, ...rest] = target.dataset.action.split(':');
      const keepsMenu = ['menu', 'panel-menu', 'dropdown'].includes(name);
      if (!keepsMenu)
      {
        closeMenu();
      }

      actions[name]?.(rest.join(':'), target);
    }
    else if (menu && !menu.element.contains(event.target))
    {
      closeMenu();
    }
  });

  document.addEventListener('dblclick', event =>
  {
    const createRow = event.target.closest('[data-create]');
    if (createRow && !event.target.closest('.tree-fold'))
    {
      S.create.selected = createRow.dataset.create;
      confirmCreate();
      return;
    }

    const row = event.target.closest('[data-open]');
    if (!row)
    {
      return;
    }

    const name = row.dataset.open;
    if (name === 'Level.ming')
    {
      openScene();
    }
    else
    {
      say(`${name} opens in the engine; this prototype has one example scene`);
    }
  });

  document.addEventListener('contextmenu', event =>
  {
    const target = event.target.closest('[data-context]');
    if (!target)
    {
      return;
    }

    event.preventDefault();
    if (target.dataset.context === 'node')
    {
      S.selected = target.dataset.id;
      S.selectedFile = '';
      draw();
      const row = document.querySelector(`[data-tree="scene"][data-id="${S.selected}"]`);
      openMenu(menuItems('node'), row, { x: event.clientX, y: event.clientY });
    }
    else
    {
      openMenu(menuItems('files'), target, { x: event.clientX, y: event.clientY });
    }
  });

  document.addEventListener('pointerover', event =>
  {
    const tipTarget = event.target.closest('[data-tip]');
    if (tipTarget && !tipTarget.contains(event.relatedTarget))
    {
      showTooltip(tipTarget);
    }

    const menuButton = event.target.closest('.menus button');
    if (menu && menuButton && menu.trigger !== menuButton && menu.trigger.closest('.menus'))
    {
      openMenu(menuItems(menuButton.dataset.action.split(':')[1]), menuButton);
    }
  });

  document.addEventListener('pointerout', event =>
  {
    if (event.target.closest('[data-tip]'))
    {
      clearTooltip();
    }
  });

  document.addEventListener('input', event =>
  {
    const target = event.target;
    if (target.matches('[data-search]'))
    {
      S.search[target.dataset.search] = target.value;
      draw();
    }
    else if (target.matches('[data-prop]'))
    {
      const node = selectedNode();
      const value = Number(target.value);
      const valid = target.value.trim() !== '' && Number.isFinite(value);
      target.closest('.field').classList.toggle('is-invalid', !valid);
      if (node && valid)
      {
        node.values[target.dataset.prop] = value;
        drawStage();
      }
    }
    else if (target.id === 'create-search')
    {
      filterCreate(target.value);
    }
    else if (target.id === 'settings-filter')
    {
      document.querySelector('#settings-list').innerHTML = settingsList(target.value);
    }
    else if (target.matches('[data-folder-name]'))
    {
      validateFolder(target);
    }
  });

  document.addEventListener('change', event =>
  {
    const target = event.target;
    if (target.matches('[data-prop]'))
    {
      const node = selectedNode();
      if (node)
      {
        markDirty();
      }
      draw();
    }
    else if (target.matches('[data-pref]'))
    {
      const value = target.value.trim();
      const key = target.dataset.pref;
      const invalid = target.dataset.kind === 'number' && !(Number(value) >= 1 && Number(value) <= 16384);
      setPref(key, invalid ? PREF_DEFAULTS[key] : value);
      if (invalid)
      {
        say('Use a value from 1 to 16384');
      }
    }
  });

  document.addEventListener('focusout', event =>
  {
    if (!isDrawing && event.target.matches('[data-rename]') && S.rename && document.hasFocus())
    {
      commitRename(event.target.value);
    }
  });

  function isTyping()
  {
    const active = document.activeElement;
    return active && active.tagName === 'INPUT';
  }

  document.addEventListener('keydown', event =>
  {
    const target = event.target;
    if (target.matches && target.matches('[data-rename]'))
    {
      if (event.key === 'Enter')
      {
        event.preventDefault();
        commitRename(target.value);
      }
      else if (event.key === 'Escape')
      {
        event.preventDefault();
        cancelRename();
      }
      return;
    }

    if (target.matches && target.matches('[data-prop]') && ['ArrowUp', 'ArrowDown'].includes(event.key))
    {
      event.preventDefault();
      const step = (event.key === 'ArrowUp' ? 1 : -1) * (event.shiftKey ? 10 : 1);
      const current = Number(target.value);
      if (Number.isFinite(current))
      {
        target.value = num(current + step);
        target.dispatchEvent(new Event('input', { bubbles: true }));
        target.dispatchEvent(new Event('change', { bubbles: true }));
      }
      return;
    }

    if (target.matches && target.matches('[data-prop], [data-pref]') && event.key === 'Enter')
    {
      target.blur();
      return;
    }

    if (target.id === 'create-search' && ['ArrowDown', 'ArrowUp', 'Enter'].includes(event.key))
    {
      event.preventDefault();
      if (event.key === 'Enter')
      {
        confirmCreate();
      }
      else
      {
        moveCreateSelection(event.key === 'ArrowDown' ? 1 : -1);
      }
      return;
    }

    if (event.key === 'Escape')
    {
      if (menu)
      {
        const trigger = menu.trigger;
        closeMenu();
        trigger.focus();
      }
      else if (S.dialog)
      {
        closeDialog();
      }
      clearTooltip();
      return;
    }

    if (menu && ['ArrowDown', 'ArrowUp', 'Home', 'End'].includes(event.key))
    {
      event.preventDefault();
      const items = [...menu.element.querySelectorAll('button:not(:disabled)')];
      const index = items.indexOf(document.activeElement);
      const count = items.length;
      const next = event.key === 'Home' ? 0 : event.key === 'End' ? count - 1 : (index + (event.key === 'ArrowDown' ? 1 : -1) + count) % count;
      items[next]?.focus();
      return;
    }

    if (S.dialog)
    {
      if (event.key === 'Tab')
      {
        const focusable = [...overlays.querySelectorAll('button:not(:disabled), input')].filter(item => item.getClientRects().length);
        const first = focusable[0];
        const last = focusable[focusable.length - 1];
        if (event.shiftKey && document.activeElement === first)
        {
          event.preventDefault();
          last?.focus();
        }
        else if (!event.shiftKey && document.activeElement === last)
        {
          event.preventDefault();
          first?.focus();
        }
      }
      return;
    }

    if (event.ctrlKey && event.key.toLowerCase() === 's')
    {
      event.preventDefault();
      if (S.sceneOpen)
      {
        save();
      }
      return;
    }

    if (event.key === 'F5')
    {
      event.preventDefault();
      actions.run('play');
      return;
    }

    if (event.key === 'F8')
    {
      event.preventDefault();
      if (S.running)
      {
        actions.run('stop');
      }
      return;
    }

    if (event.ctrlKey && event.key.toLowerCase() === 'a' && !isTyping() && S.page === 'editor' && S.sceneOpen)
    {
      event.preventDefault();
      openCreateNode();
      return;
    }

    if (isTyping() || event.ctrlKey || event.altKey || event.metaKey)
    {
      return;
    }

    const inSceneTree = target.closest && target.closest('[data-tree="scene"]');
    if (event.key === 'F2')
    {
      event.preventDefault();
      beginRename();
    }
    else if (event.key === 'Delete' && selectedNode() && selectedNode().id !== S.scene.id)
    {
      event.preventDefault();
      openDialog('delete');
    }
    else if (inSceneTree && event.key === 'ArrowDown')
    {
      event.preventDefault();
      moveTreeSelection(1);
    }
    else if (inSceneTree && event.key === 'ArrowUp')
    {
      event.preventDefault();
      moveTreeSelection(-1);
    }
    else if (inSceneTree && ['ArrowLeft', 'ArrowRight'].includes(event.key))
    {
      event.preventDefault();
      const id = S.selected;
      if (event.key === 'ArrowLeft')
      {
        S.foldedNodes.add(id);
      }
      else
      {
        S.foldedNodes.delete(id);
      }
      draw();
      document.querySelector(`[data-tree="scene"][data-id="${id}"]`)?.focus();
    }
    else if (S.page === 'editor' && ['q', 'w', 'e', 'r'].includes(event.key.toLowerCase()))
    {
      S.tool = { q: 'select', w: 'move', e: 'rotate', r: 'scale' }[event.key.toLowerCase()];
      draw();
    }
  });

  /* Pointer drags: seams and the canvas. */
  let drag = null;

  document.addEventListener('pointerdown', event =>
  {
    const seam = event.target.closest('[data-seam]');
    if (seam)
    {
      event.preventDefault();
      const workspace = document.querySelector('#workspace');
      const key = seam.dataset.seam;
      seam.classList.add('is-dragging');
      drag = { kind: 'seam', key, seam, workspace, startX: event.clientX, startY: event.clientY, startValue: S.layout[key] };
      return;
    }

    const canvas = event.target.closest('#canvas');
    if (!canvas || S.workspace !== '2D' || event.target.closest('button'))
    {
      return;
    }

    if (event.button === 1)
    {
      event.preventDefault();
      drag = { kind: 'pan', startX: event.clientX, startY: event.clientY, panX: S.pan.x, panY: S.pan.y };
      return;
    }

    if (event.button !== 0 || !S.sceneOpen)
    {
      return;
    }

    const hit = event.target.closest('[data-node]');
    S.selectedFile = '';
    S.selected = hit ? hit.dataset.node : '';
    if (S.inspectorTab === 'Import')
    {
      S.inspectorTab = 'Inspector';
    }
    draw();

    const node = selectedNode();
    if (hit && node)
    {
      const v = node.values;
      drag = { kind: 'node', node, startX: event.clientX, startY: event.clientY, x: v.x, y: v.y, rotation: v.rotation, scaleX: v.scaleX, scaleY: v.scaleY, moved: false };
    }
  });

  document.addEventListener('pointermove', event =>
  {
    const canvas = document.querySelector('#canvas');
    if (canvas)
    {
      const point = localPoint(event, canvas);
      const inside = event.target.closest && event.target.closest('#canvas');
      updateCursor(inside ? point : null);
    }

    if (!drag)
    {
      return;
    }

    const scale = uiScale();
    const dx = (event.clientX - drag.startX) / scale;
    const dy = (event.clientY - drag.startY) / scale;
    if (drag.kind === 'seam')
    {
      const shellHeight = drag.workspace.clientHeight;
      const limits = { left: [170, 420], right: [200, 460], bottom: [60, shellHeight - 200], leftSplit: [100, shellHeight - 130] }[drag.key];
      const direction = drag.key === 'right' || drag.key === 'bottom' ? -1 : 1;
      const delta = drag.key === 'left' || drag.key === 'right' ? dx : dy;
      S.layout[drag.key] = clamp(drag.startValue + delta * direction, limits[0], limits[1]);
      const names = { left: '--left-width', right: '--right-width', bottom: '--bottom-height', leftSplit: '--left-split' };
      drag.workspace.style.setProperty(names[drag.key], `${S.layout[drag.key]}px`);
    }
    else if (drag.kind === 'pan')
    {
      S.pan.x = drag.panX + dx;
      S.pan.y = drag.panY + dy;
      syncCanvas();
    }
    else if (drag.kind === 'node')
    {
      if (!drag.moved && Math.abs(dx) + Math.abs(dy) < 3)
      {
        return;
      }

      drag.moved = true;
      const v = drag.node.values;
      const zoom = S.zoom / 100;
      const grid = Number(S.prefs.gridSize);
      if (S.tool === 'rotate')
      {
        const angle = drag.rotation + dx * 0.5;
        v.rotation = Math.round((S.prefs.snap ? Math.round(angle / 15) * 15 : angle) * 10) / 10;
      }
      else if (S.tool === 'scale')
      {
        const factor = Math.max(0.1, 1 + (dx - dy) / 120);
        v.scaleX = Math.round(drag.scaleX * factor * 100) / 100;
        v.scaleY = Math.round(drag.scaleY * factor * 100) / 100;
      }
      else
      {
        const x = drag.x + dx / zoom;
        const y = drag.y + dy / zoom;
        v.x = Math.round(S.prefs.snap ? Math.round(x / grid) * grid : x);
        v.y = Math.round(S.prefs.snap ? Math.round(y / grid) * grid : y);
      }
      drawStage();
      ['x', 'y', 'rotation', 'scaleX', 'scaleY'].forEach(key =>
      {
        const field = document.querySelector(`[data-prop="${key}"]`);
        if (field)
        {
          field.value = num(v[key]);
        }
      });
    }
  });

  document.addEventListener('pointerup', () =>
  {
    if (!drag)
    {
      return;
    }

    const finished = drag;
    drag = null;
    if (finished.kind === 'seam')
    {
      finished.seam.classList.remove('is-dragging');
    }
    else if (finished.kind === 'node' && finished.moved)
    {
      markDirty();
      draw();
    }
  });

  document.addEventListener('wheel', event =>
  {
    const canvas = event.target.closest && event.target.closest('#canvas');
    if (canvas)
    {
      event.preventDefault();
      stepZoom(event.deltaY < 0 ? 1 : -1, localPoint(event, canvas));
    }
  }, { passive: false });

  /* Tooltip */
  let tooltipTimer;

  function clearTooltip()
  {
    clearTimeout(tooltipTimer);
    document.querySelector('.tooltip')?.remove();
  }

  function showTooltip(target)
  {
    clearTooltip();
    if (!S.prefs.tooltips)
    {
      return;
    }

    tooltipTimer = setTimeout(() =>
    {
      if (!target.isConnected)
      {
        return;
      }

      const tip = document.createElement('div');
      tip.className = 'tooltip';
      tip.setAttribute('role', 'tooltip');
      tip.textContent = target.dataset.tip;
      document.body.append(tip);

      const scale = uiScale();
      const rect = target.getBoundingClientRect();
      const ratio = rect.width / (target.offsetWidth || 1) || 1;
      const left = Math.min(rect.left / ratio, innerWidth / scale - tip.offsetWidth - 6);
      const below = rect.bottom / ratio + 4;
      const top = below + tip.offsetHeight + 6 < innerHeight / scale ? below : rect.top / ratio - tip.offsetHeight - 4;
      tip.style.left = `${Math.max(4, left)}px`;
      tip.style.top = `${top}px`;
    }, 450);
  }

  /* Start */
  draw();
  if (query.get('dialog') === 'settings')
  {
    actions.settings('Editor');
  }
})();
