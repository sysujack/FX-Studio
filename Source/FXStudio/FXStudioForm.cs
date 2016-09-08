﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.IO;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using WeifenLuo.WinFormsUI.Docking;

namespace FXStudio
{
    public partial class FXStudioForm : Form
    {
        private MessageHandler m_messageHandler;
        private DeserializeDockContent m_dockContent;
        private AssetsView m_assetsView;
        private ProjectView m_projectView;
        private RenderView m_renderViw;
        private PropertiesView m_propertiesView;
        private OutputView m_outputView;

        private readonly ToolStripRenderer renderEx = new ToolStripExRenderer();

        public FXStudioForm()
        {
            InitializeComponent();

            CreateStandardViews();
            m_dockContent = new DeserializeDockContent(GetContentFromString);

            toolStripEx.DefaultRenderer = renderEx;
            SetScheme();

            try
            {
                IntPtr hInstance = Marshal.GetHINSTANCE(this.GetType().Module);
                IntPtr hwnd = this.panelAllView.Handle;
                //                 RenderMethods.CreateInstance(hInstance, IntPtr.Zero, hwnd, 1, this.panelAllView.Width, this.panelAllView.Height);
                m_messageHandler = new MessageHandler(this.panelAllView, this);
            }
            catch (Exception e)
            {
                MessageBox.Show("Error: " + e.ToString());
            }
        }

        public MessageHandler GetMessageHandler()
        {
            return m_messageHandler;
        }

        private void ShutDown()
        {
            //             RenderMethods.DestroyInstance();
            Application.Exit();
        }

        private void CreateStandardViews()
        {
            m_assetsView = new AssetsView();
            m_projectView = new ProjectView();
            m_renderViw = new RenderView();
            m_propertiesView = new PropertiesView();
            m_outputView = new OutputView();
        }

        private void DestoryStandardViews()
        {
            m_assetsView.DockPanel = null;
            m_projectView.DockPanel = null;
            m_renderViw.DockPanel = null;
            m_propertiesView.DockPanel = null;
            m_outputView.DockPanel = null;

            foreach (IDockContent document in panelAllView.DocumentsToArray())
            {
                document.DockHandler.DockPanel = null;
                document.DockHandler.Close();
            }

            System.Diagnostics.Debug.Assert(panelAllView.Panes.Count == 0);
        }

        private IDockContent GetContentFromString(string viewString)
        {
            if (viewString == typeof(AssetsView).ToString())
            {
                return m_assetsView;
            }
            else if (viewString == typeof(ProjectView).ToString())
            {
                return m_projectView;
            }
            else if (viewString == typeof(RenderView).ToString())
            {
                return m_projectView;
            }
            else if (viewString == typeof(PropertiesView).ToString())
            {
                return m_projectView;
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
            panelTop.Visible = true;
            panelBottom.Visible = true;
            panelTop.BackColor = panelAllView.Theme.Skin.ColorPalette.MainWindowActive.Background;
            panelBottom.BackColor = panelAllView.Theme.Skin.ColorPalette.MainWindowActive.Background;
            statusStripMain.BackColor = panelAllView.Theme.Skin.ColorPalette.MainWindowStatusBarDefault.Background;
        }

        private void ResetLayout()
        {
            panelAllView.SuspendLayout(true);
            DestoryStandardViews();
            CreateStandardViews();

            m_assetsView.Show(panelAllView, DockState.DockLeft);
            m_projectView.Show(panelAllView, DockState.DockLeft);
            m_propertiesView.Show(panelAllView, DockState.DockRight);

            EditorView doc1 = CreateNewDocument("Document1");
            EditorView doc2 = CreateNewDocument("Document2");
            doc1.Show(panelAllView, DockState.Document);
            doc2.Show(doc1.Pane, null);

            m_renderViw.Show(panelAllView, DockState.Document);
            m_outputView.Show(panelAllView, DockState.Float);
            m_outputView.Show(m_renderViw.Pane, DockAlignment.Bottom, 0.35);

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
        }

        private IDockContent FindDocument(string text)
        {
            if (panelAllView.DocumentStyle == DocumentStyle.SystemMdi)
            {
                foreach (Form form in MdiChildren)
                    if (form.Text == text)
                        return form as IDockContent;

                return null;
            }
            else
            {
                foreach (IDockContent content in panelAllView.Documents)
                    if (content.DockHandler.TabText == text)
                        return content;

                return null;
            }
        }

        private EditorView CreateNewDocument()
        {
            EditorView dummyDoc = new EditorView();

            int count = 1;
            string text = $"Document{count}";
            while (FindDocument(text) != null)
            {
                count++;
                text = $"Document{count}";
            }

            dummyDoc.Text = text;
            return dummyDoc;
        }

        private EditorView CreateNewDocument(string text)
        {
            EditorView dummyDoc = new EditorView();
            dummyDoc.Text = text;
            return dummyDoc;
        }
    }
}
