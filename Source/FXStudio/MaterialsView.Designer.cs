﻿namespace FXStudio
{
    partial class MaterialsView
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.splitContainer1 = new System.Windows.Forms.SplitContainer();
            this.panelMaterial = new System.Windows.Forms.Panel();
            this.toolStripMaterial = new System.Windows.Forms.ToolStrip();
            this.panelTexture = new System.Windows.Forms.Panel();
            this.toolStripTexture = new System.Windows.Forms.ToolStrip();
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).BeginInit();
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            this.SuspendLayout();
            // 
            // splitContainer1
            // 
            this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer1.Location = new System.Drawing.Point(0, 0);
            this.splitContainer1.Name = "splitContainer1";
            this.splitContainer1.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.Controls.Add(this.panelMaterial);
            this.splitContainer1.Panel1.Controls.Add(this.toolStripMaterial);
            this.splitContainer1.Panel1MinSize = 200;
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.Controls.Add(this.panelTexture);
            this.splitContainer1.Panel2.Controls.Add(this.toolStripTexture);
            this.splitContainer1.Panel2MinSize = 300;
            this.splitContainer1.Size = new System.Drawing.Size(366, 702);
            this.splitContainer1.SplitterDistance = 271;
            this.splitContainer1.TabIndex = 0;
            // 
            // panelMaterial
            // 
            this.panelMaterial.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panelMaterial.Location = new System.Drawing.Point(0, 25);
            this.panelMaterial.Name = "panelMaterial";
            this.panelMaterial.Size = new System.Drawing.Size(366, 246);
            this.panelMaterial.TabIndex = 1;
            this.panelMaterial.Resize += new System.EventHandler(this.panelMaterial_Resize);
            // 
            // toolStripMaterial
            // 
            this.toolStripMaterial.Location = new System.Drawing.Point(0, 0);
            this.toolStripMaterial.Name = "toolStripMaterial";
            this.toolStripMaterial.Size = new System.Drawing.Size(366, 25);
            this.toolStripMaterial.TabIndex = 0;
            this.toolStripMaterial.Text = "toolStrip1";
            // 
            // panelTexture
            // 
            this.panelTexture.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panelTexture.Location = new System.Drawing.Point(0, 25);
            this.panelTexture.Name = "panelTexture";
            this.panelTexture.Size = new System.Drawing.Size(366, 402);
            this.panelTexture.TabIndex = 1;
            // 
            // toolStripTexture
            // 
            this.toolStripTexture.Location = new System.Drawing.Point(0, 0);
            this.toolStripTexture.Name = "toolStripTexture";
            this.toolStripTexture.Size = new System.Drawing.Size(366, 25);
            this.toolStripTexture.TabIndex = 0;
            this.toolStripTexture.Text = "toolStrip2";
            // 
            // MaterialsView
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(366, 702);
            this.Controls.Add(this.splitContainer1);
            this.Name = "MaterialsView";
            this.TabText = "Materials";
            this.Text = "Materials";
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel1.PerformLayout();
            this.splitContainer1.Panel2.ResumeLayout(false);
            this.splitContainer1.Panel2.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).EndInit();
            this.splitContainer1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.SplitContainer splitContainer1;
        private System.Windows.Forms.Panel panelMaterial;
        private System.Windows.Forms.ToolStrip toolStripMaterial;
        private System.Windows.Forms.Panel panelTexture;
        private System.Windows.Forms.ToolStrip toolStripTexture;
    }
}