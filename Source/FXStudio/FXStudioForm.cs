﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.Linq;
using System.Xml;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using System.Configuration;
using WeifenLuo.WinFormsUI.Docking;

namespace FXStudio
{
    public delegate void UpdatePropertiesDelegate(XmlNode selectedNode);
    public delegate void ChangeMaterialDelegate(string name, uint actorId, int mesh);
    public delegate void MoveActorDelegate(string component, string attribute, Inspector.Vector3 value);
    public delegate void ModifyMaterialDelegate(string name, string value);
    public delegate void UpdateOutputDelegate(string output, string error);
    public delegate void OpenEffectFile(XmlNode effectNode);

    public delegate bool DllProgressCallback(Single percent, string error);
    public delegate void DllMoveDelegate(string actorXml);

    public partial class FXStudioForm : Form
    {
        private MessageHandler m_messageHandler;
        private string m_DefaultLocation;
        private string m_ProjectLocation;
        private bool m_Loaded = false;

        private DllMoveDelegate m_DllMoveDelegate;

        private DeserializeDockContent m_dockContent;
        private MaterialsView m_MaterialView;
        private AssetsView m_AssetsView;
        private ProjectView m_ProjectView;
        private EditorView m_EditorView;
        private RenderView m_RenderView;
        private PropertiesView m_PropertiesView;
        private TaskListView m_taskView;
        private OutputView m_outputView;

        private readonly ToolStripRenderer renderEx = new ToolStripExRenderer();

        public FXStudioForm()
        {
            m_DefaultLocation = Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments) + @"\FX Studio\Projects";
            if (!Directory.Exists(m_DefaultLocation))
            {
                Directory.CreateDirectory(m_DefaultLocation);
            }
            m_DllMoveDelegate = new DllMoveDelegate(ModifyTransformXml);

            InitializeComponent();

            CreateStandardViews();
            m_dockContent = new DeserializeDockContent(GetContentFromString);
            toolStripEx.DefaultRenderer = renderEx;
            SetScheme();

            var panel = m_RenderView.GetRenderPanel();
            if (panel != null)
            {
                m_messageHandler = new MessageHandler(this, m_RenderView.GetRenderPanel());
            }
        }

        public MessageHandler GetMessageHandler()
        {
            return m_messageHandler;
        }

        public string GetDefaultLocation()
        {
            return m_DefaultLocation;
        }

        public void PickActor(Point cursor)
        {
            int mesh = -1;
            uint actorId = RenderMethods.GetPickedActor(cursor.X, cursor.Y, ref mesh);
            m_ProjectView.SelectActorNode(actorId);
        }

        public bool ProjectLoaded { get { return m_Loaded; } }

        private void OpenProject(string project)
        {
            RenderMethods.OpenProject(project);
            m_ProjectLocation = Path.GetDirectoryName(project);

            m_ProjectView.UpdateProject(project, selectedNode =>
            {
                if (m_PropertiesView.UpdateProjectProperties(selectedNode))
                {
                    int selectAcotrId = m_ProjectView.GetSelectActorId();
                    if (selectAcotrId > 2)
                        RenderMethods.SetPickedActor((uint)selectAcotrId);
                    else
                        RenderMethods.SetPickedActor(0);
                }
                else
                    RenderMethods.SetPickedActor(0);
            });

            string assetFile = m_ProjectLocation + @"\" + Path.GetFileNameWithoutExtension(project) + ".asset";
            m_AssetsView.UpdateAssets(assetFile, m_PropertiesView.UpdateAssetProperties, m_outputView.UpdateCompileInfo,
                effectNode =>
                {
                    panelAllView.ActiveDocumentPane.ActiveContent = m_EditorView;
                    m_EditorView.ShowEffectDoc(effectNode, m_ProjectLocation);
                },
                m_MaterialView);

            m_PropertiesView.ProjectLocation = m_ProjectLocation;
            m_PropertiesView.SetMoveActorDelegate(
                (string component, string attribute, Inspector.Vector3 value) =>
                {
                    int actorId = m_ProjectView.GetSelectActorId();

                    XmlDocument xmlDoc = new XmlDocument();
                    XmlElement xmlActor = xmlDoc.CreateElement("Actor");

                    xmlActor.Attributes.Append(XmlUtility.CreateAttribute(xmlDoc, "id", (actorId > 2 ? actorId : 0).ToString()));

                    XmlElement transformComponent = xmlDoc.CreateElement(component); ;
                    XmlElement transformType = xmlDoc.CreateElement(attribute);
                    transformComponent.AppendChild(transformType);
                    transformType.Attributes.Append(XmlUtility.CreateAttribute(xmlDoc, "x", value.X.ToString()));
                    transformType.Attributes.Append(XmlUtility.CreateAttribute(xmlDoc, "y", value.Y.ToString()));
                    transformType.Attributes.Append(XmlUtility.CreateAttribute(xmlDoc, "z", value.Z.ToString()));
                    xmlActor.AppendChild(transformComponent);

                    RenderMethods.ModifyActor(xmlActor.OuterXml);
                },
                (string name, string value) =>
                {
                    m_AssetsView.ModifyMaterial(name, value);
                }
            );

            m_RenderView.SetChangeMaterialDelegate((name, actorId, mesh) =>
            {
                m_ProjectView.SelectActorNode(actorId);
                XmlNode actorNode = m_ProjectView.GetSelectActorXml();

                XmlDocument xmlDoc = new XmlDocument();
                XmlElement xmlActor = xmlDoc.CreateElement("Actor");

                xmlActor.Attributes.Append(XmlUtility.CreateAttribute(xmlDoc, "id", actorId.ToString()));

                XmlElement material = xmlDoc.CreateElement("Material");
                material.InnerText = @"Materials\" + Path.GetFileName(name);

                XmlElement renderComponent = null;
                string nodeType = actorNode.Attributes["type"].Value;
                if (nodeType == "Model")
                {
                    renderComponent = xmlDoc.CreateElement("ModelRenderComponent");
                    material.Attributes.Append(XmlUtility.CreateAttribute(xmlDoc, "index", mesh.ToString()));
                    XmlElement materials = xmlDoc.CreateElement("Materials");
                    materials.AppendChild(material);
                    renderComponent.AppendChild(materials);
                }
                else if (nodeType == "Plane")
                {
                    renderComponent = xmlDoc.CreateElement("PlaneRenderComponent");
                    renderComponent.AppendChild(material);
                }
                else if (nodeType == "Sphere")
                {
                    renderComponent = xmlDoc.CreateElement("SphereRenderComponent");
                    renderComponent.AppendChild(material);
                }
                else if (nodeType == "Torus")
                {
                    renderComponent = xmlDoc.CreateElement("TorusRenderComponent");
                    renderComponent.AppendChild(material);
                }
                else if (nodeType == "Teapot")
                {
                    renderComponent = xmlDoc.CreateElement("TeapotRenderComponent");
                    renderComponent.AppendChild(material);
                }

                xmlActor.AppendChild(renderComponent);

                RenderMethods.ModifyActor(xmlActor.OuterXml);
                m_ProjectView.ModifyMaterialXml((int)actorId, mesh, material.InnerText);
            });

            RenderMethods.SetMoveDelegate(m_DllMoveDelegate);

            EnableControlView(true);
        }

        private void ModifyTransformXml(string actorXml)
        {
            m_ProjectView.ModifyTransformXml(actorXml);
        }

        private void EnableControlView(bool enable)
        {
            toolStripButtonImport.Enabled = enable;
            toolStripButtonSaveAll.Enabled = enable;
            toolStripButtonUndo.Enabled = enable;
            toolStripButtonRedo.Enabled = enable;
            toolStripButtonEffect.Enabled = enable;
            toolStripButtonReBuild.Enabled = enable;
            toolStripButtonCompile.Enabled = enable;
            toolStripButtonTeapot.Enabled = enable;
            toolStripButtonSphere.Enabled = enable;
            toolStripButtonTorus.Enabled = enable;
            toolStripButtonPlane.Enabled = enable;
            toolStripButtonDirectional.Enabled = enable;
            toolStripButtonPoint.Enabled = enable;
            toolStripButtonSpot.Enabled = enable;
            toolStripButtonPlay.Enabled = enable;
            toolStripButtonStop.Enabled = enable;

            m_Loaded = enable;
        }

        private void CreateStandardViews()
        {
            m_MaterialView = new MaterialsView();
            m_AssetsView = new AssetsView();
            m_ProjectView = new ProjectView();
            m_PropertiesView = new PropertiesView();
            m_outputView = new OutputView();
            m_taskView = new TaskListView();
            m_EditorView = new EditorView();
            m_RenderView = new RenderView();
        }

        private void DestoryStandardViews()
        {
            m_MaterialView.DockPanel = null;
            m_AssetsView.DockPanel = null;
            m_ProjectView.DockPanel = null;
            m_RenderView.DockPanel = null;
            m_PropertiesView.DockPanel = null;
            m_outputView.DockPanel = null;
            m_taskView.DockPanel = null;
            m_EditorView.DockPanel = null;

            foreach (IDockContent document in panelAllView.DocumentsToArray())
            {
                document.DockHandler.DockPanel = null;
                document.DockHandler.Close();
            }

            System.Diagnostics.Debug.Assert(panelAllView.Panes.Count == 0);
        }

        private IDockContent GetContentFromString(string viewString)
        {
            if (viewString == typeof(MaterialsView).ToString())
            {
                return m_MaterialView;
            }
            if (viewString == typeof(AssetsView).ToString())
            {
                return m_AssetsView;
            }
            else if (viewString == typeof(ProjectView).ToString())
            {
                return m_ProjectView;
            }
            else if (viewString == typeof(EditorView).ToString())
            {
                return m_EditorView;
            }
            else if (viewString == typeof(RenderView).ToString())
            {
                return m_RenderView;
            }
            else if (viewString == typeof(PropertiesView).ToString())
            {
                return m_PropertiesView;
            }
            else if (viewString == typeof(OutputView).ToString())
            {
                return m_outputView;
            }
            else if (viewString == typeof(TaskListView).ToString())
            {
                return m_taskView;
            }
            else
            {
                return null;
            }
        }

        private void SetScheme()
        {
            toolStripEx.SetStyle(this.menuStripMain);
            toolStripEx.SetStyle(this.toolStripMain);
            panelTop.BackColor = panelAllView.Theme.ColorPalette.MainWindowActive.Background;
            panelBottom.BackColor = panelAllView.Theme.ColorPalette.MainWindowActive.Background;
            statusStripMain.BackColor = panelAllView.Theme.ColorPalette.MainWindowStatusBarDefault.Background;
        }

        private void ResetLayout()
        {
            panelAllView.SuspendLayout(true);
            DestoryStandardViews();
            CreateStandardViews();

            m_MaterialView.Show(panelAllView, DockState.DockLeft);
            m_AssetsView.Show(panelAllView, DockState.DockLeft);
            m_ProjectView.Show(panelAllView, DockState.DockLeft);
            m_PropertiesView.Show(panelAllView, DockState.DockRight);
            m_EditorView.Show(panelAllView, DockState.Document);
            m_RenderView.Show(panelAllView, DockState.Document);
            m_taskView.Show(m_RenderView.Pane, DockAlignment.Bottom, 0.15);
            m_outputView.Show(m_taskView.Pane, null);

            panelAllView.ResumeLayout(true, true);
        }

        private void FXStudioForm_Load(object sender, EventArgs e)
        {
            string configFile = Path.Combine(Path.GetDirectoryName(Application.ExecutablePath), "FXStudio.Layout.config");

            if (File.Exists(configFile))
            {
                panelAllView.LoadFromXml(configFile, m_dockContent);
            }
            else
            {
                ResetLayout();
            }

            panelAllView.DockLeftPortion = panelAllView.Width * 0.20d;
            panelAllView.DockRightPortion = panelAllView.DockLeftPortion;

            IntPtr hInstance = Marshal.GetHINSTANCE(this.GetType().Module);
            var panelRender = m_RenderView.GetRenderPanel();
            m_messageHandler.ResetRenderPanel(panelRender);

            try
            {
                if (!RenderMethods.CreateInstance(hInstance, IntPtr.Zero, panelRender.Handle, 1, panelRender.Width, panelRender.Height))
                    this.Close();

                if (panelRender.Width != 0 && panelRender.Height != 0)
                    RenderMethods.ResizeWnd(panelRender.Width, panelRender.Height);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
                this.Close();
            }
        }

        private void FXStudioForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            string configFile = Path.Combine(Path.GetDirectoryName(Application.ExecutablePath), "FXStudio.Layout.config");
            panelAllView.SaveAsXml(configFile);
        }

        private void toolStripButtonOpen_Click(object sender, EventArgs e)
        {
            OpenFileDialog dialog = new OpenFileDialog();

            dialog.InitialDirectory = m_DefaultLocation;
            dialog.Filter = "FX Studio files (*.fxsproj)|*.fxsproj";
            dialog.FilterIndex = 1;
            dialog.RestoreDirectory = true;

            if (dialog.ShowDialog() == DialogResult.OK)
            {
                OpenProject(dialog.FileName);
            }
        }

        private void FXStudioForm_FormClosed(object sender, FormClosedEventArgs e)
        {
            try
            {
                RenderMethods.DestroyInstance();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }

        private void FXStudioForm_Shown(object sender, EventArgs e)
        {
            Configuration appConfig = ConfigurationManager.OpenExeConfiguration(ConfigurationUserLevel.None);

            bool isShow = true;
            if (appConfig.AppSettings.Settings["show"] != null)
            {
                isShow = bool.Parse(appConfig.AppSettings.Settings["show"].Value);
            }
            else
            {
                appConfig.AppSettings.Settings.Add("show", bool.TrueString);
                appConfig.Save(ConfigurationSaveMode.Modified);
                isShow = true;
            }

            if (isShow)
            {
                StartPageDialog startForm = new StartPageDialog("", m_DefaultLocation);
                if (startForm.ShowDialog(this) == DialogResult.OK)
                {
                    OpenProject(startForm.GetProjectPath());
                }
            }
        }

        private void FXStudioForm_Resize(object sender, EventArgs e)
        {
            if (panelAllView.Width != 0 && panelAllView.Height != 0)
            {
                panelAllView.DockLeftPortion = panelAllView.Width * 0.20d;
                panelAllView.DockRightPortion = panelAllView.DockLeftPortion;
            }
        }

        private void toolStripButtonNew_Click(object sender, EventArgs e)
        {
            NewProjectDialog dialog = new NewProjectDialog(m_DefaultLocation);
            if (dialog.ShowDialog() == DialogResult.OK)
            {
                string location = dialog.GetProjectLocation();
                Directory.CreateDirectory(location);
                string projectFile = location + @"\" + dialog.GetProjectName() + @".fxsproj";
                string assetFile = location + @"\" + dialog.GetProjectName() + @".asset";

                File.WriteAllText(projectFile, XmlUtility.DefaultProjectXml);
                File.WriteAllText(assetFile, XmlUtility.DefaultAssetXml);

                string sourcePath = Directory.GetCurrentDirectory() + @"\Data";
                StartPageDialog.CopyDefaultData(sourcePath, location);

                OpenProject(projectFile);
            }
        }

        private void toolStripButtonTeapot_Click(object sender, EventArgs e)
        {
            XmlDocument xmlDoc = new XmlDocument();
            XmlElement geometryElement = xmlDoc.CreateElement("Teapot");

            geometryElement.Attributes.Append(XmlUtility.CreateAttribute(xmlDoc, "type", "Teapot"));
            geometryElement.AppendChild(XmlUtility.CreateTransformComponent(xmlDoc, "0", "0"));
            geometryElement.AppendChild(XmlUtility.CreateTeapotRenderComponent(xmlDoc));

            if (RenderMethods.AddActor(geometryElement.OuterXml) > 0)
            {
                m_ProjectView.AddActorNode(geometryElement);
            }
        }

        private void toolStripButtonSphere_Click(object sender, EventArgs e)
        {
            XmlDocument xmlDoc = new XmlDocument();
            XmlElement geometryElement = xmlDoc.CreateElement("Sphere");

            geometryElement.Attributes.Append(XmlUtility.CreateAttribute(xmlDoc, "type", "Sphere"));
            geometryElement.AppendChild(XmlUtility.CreateTransformComponent(xmlDoc, "0", "0"));
            geometryElement.AppendChild(XmlUtility.CreateSphereRenderComponent(xmlDoc));

            if (RenderMethods.AddActor(geometryElement.OuterXml) > 0)
            {
                m_ProjectView.AddActorNode(geometryElement);
            }
        }

        private void toolStripButtonTorus_Click(object sender, EventArgs e)
        {
            XmlDocument xmlDoc = new XmlDocument();
            XmlElement geometryElement = xmlDoc.CreateElement("Torus");

            geometryElement.Attributes.Append(XmlUtility.CreateAttribute(xmlDoc, "type", "Torus"));
            geometryElement.AppendChild(XmlUtility.CreateTransformComponent(xmlDoc, "0", "0"));
            geometryElement.AppendChild(XmlUtility.CreateTorusRenderComponent(xmlDoc));

            if (RenderMethods.AddActor(geometryElement.OuterXml) > 0)
            {
                m_ProjectView.AddActorNode(geometryElement);
            }
        }

        private void toolStripButtonPlane_Click(object sender, EventArgs e)
        {
            XmlDocument xmlDoc = new XmlDocument();
            XmlElement geometryElement = xmlDoc.CreateElement("Plane");

            geometryElement.Attributes.Append(XmlUtility.CreateAttribute(xmlDoc, "type", "Plane"));
            geometryElement.AppendChild(XmlUtility.CreateTransformComponent(xmlDoc, "0", "0"));
            geometryElement.AppendChild(XmlUtility.CreatePlaneRenderComponent(xmlDoc));

            if (RenderMethods.AddActor(geometryElement.OuterXml) > 0)
            {
                m_ProjectView.AddActorNode(geometryElement);
            }
        }

        private void toolStripButtonImport_Click(object sender, EventArgs e)
        {
            OpenFileDialog dialog = new OpenFileDialog();

            dialog.Filter = "All supported formats (*.fbx; *.dae; *.blend; *.3ds; *.ase; *.obj; *.ply; *.x; *.ms3d;)|" +
                "*.fbx; *.dae; *.blend; *.3ds; *.ase; *.obj; *.ply; *.x; *.ms3d;|" +
                "Autodesk FBX (*.fbx)|*.fbx|" + "Autodesk 3DS (*.3ds)|*.3ds|" + "Autodesk ASE (*.ase)|*.ase|" +
                "Collada Model (*.dae)|*.dae|" + "Blender 3D (*.blend)|*.blend|" + "Stanford Polygon Library (*.ply)|*.ply|" +
                "Wavefront Object (*.obj)|*.obj|" + "Milkshape 3D (*.ms3d)|*.ms3d";
            dialog.FilterIndex = 1;
            dialog.RestoreDirectory = true;

            if (dialog.ShowDialog() == DialogResult.OK)
            {
                string destFileName = m_ProjectLocation + @"\Models\" + Path.GetFileNameWithoutExtension(dialog.FileName) + ".model";
                if (!Directory.Exists(Path.GetDirectoryName(destFileName)))
                {
                    Directory.CreateDirectory(Path.GetDirectoryName(destFileName));
                }

                LoadingProgressDialog loadDlg = new LoadingProgressDialog();
                loadDlg.SourceFileName = dialog.FileName;
                loadDlg.DestFileName = destFileName;
                if (loadDlg.ShowDialog() == DialogResult.OK)
                {
                    string fileName = Path.GetFileNameWithoutExtension(destFileName);
                    fileName = fileName.First().ToString().ToUpper() + fileName.Substring(1);
                    XmlDocument xmlDoc = new XmlDocument();
                    XmlElement modelElement = xmlDoc.CreateElement(fileName);

                    modelElement.Attributes.Append(XmlUtility.CreateAttribute(xmlDoc, "type", "Model"));
                    modelElement.AppendChild(XmlUtility.CreateTransformComponent(xmlDoc));
                    modelElement.AppendChild(XmlUtility.CreateModelRenderComponent(xmlDoc, destFileName));

                    if (RenderMethods.AddActor(modelElement.OuterXml) > 0)
                    {
                        m_ProjectView.AddActorNode(modelElement);
                    }
                }
            }
        }

        private void toolStripButtonEffect_Click(object sender, EventArgs e)
        {
            m_AssetsView.AddEffect();
        }

        private void toolStripButtonSaveAll_Click(object sender, EventArgs e)
        {
            m_ProjectView.SaveProjectFile();
            m_AssetsView.SaveAssetsFile();
        }

        private void toolStripButtonReBuild_Click(object sender, EventArgs e)
        {

        }

        private void toolStripButtonCompile_Click(object sender, EventArgs e)
        {
            XmlNode effectNode = m_EditorView.GetSelectedDocument();
            if (effectNode == null)
            {
                MessageBox.Show(@"Please open an effect file in ""Editor"" pane!", "FX Studio");
                return;
            }
            m_AssetsView.ModifyEffect(effectNode);
        }

        private void toolStripMenuItemUndo_Click(object sender, EventArgs e)
        {

        }

        private void toolStripMenuItemRedo_Click(object sender, EventArgs e)
        {

        }

    }
}
