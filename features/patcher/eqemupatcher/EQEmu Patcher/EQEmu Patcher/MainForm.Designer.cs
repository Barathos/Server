namespace EQEmu_Patcher
{
    partial class MainForm
    {
        private System.ComponentModel.IContainer components = null;

        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MainForm));
            this.mainShell = new System.Windows.Forms.TableLayoutPanel();
            this.heroPanel = new System.Windows.Forms.Panel();
            this.splashLogo = new System.Windows.Forms.PictureBox();
            this.lblHeroKicker = new System.Windows.Forms.Label();
            this.lblHeroTitle = new System.Windows.Forms.Label();
            this.lblHeroSubtitle = new System.Windows.Forms.Label();
            this.sidebarPanel = new System.Windows.Forms.Panel();
            this.lblServiceHeading = new System.Windows.Forms.Label();
            this.lblServiceStatus = new System.Windows.Forms.Label();
            this.lblServiceMessage = new System.Windows.Forms.Label();
            this.lblClientHeading = new System.Windows.Forms.Label();
            this.lblClientValue = new System.Windows.Forms.Label();
            this.lblManifestHeading = new System.Windows.Forms.Label();
            this.lblManifestValue = new System.Windows.Forms.Label();
            this.lblDownloadHeading = new System.Windows.Forms.Label();
            this.lblDownloadValue = new System.Windows.Forms.Label();
            this.lblLastPatchHeading = new System.Windows.Forms.Label();
            this.lblLastPatchValue = new System.Windows.Forms.Label();
            this.contentPanel = new System.Windows.Forms.Panel();
            this.headerPanel = new System.Windows.Forms.Panel();
            this.lblTitle = new System.Windows.Forms.Label();
            this.lblSubtitle = new System.Windows.Forms.Label();
            this.lblStatusBadge = new System.Windows.Forms.Label();
            this.lblVersion = new System.Windows.Forms.Label();
            this.tabContent = new System.Windows.Forms.TabControl();
            this.tabPatchNotes = new System.Windows.Forms.TabPage();
            this.txtPatchNotes = new System.Windows.Forms.RichTextBox();
            this.tabPatchLog = new System.Windows.Forms.TabPage();
            this.txtList = new System.Windows.Forms.TextBox();
            this.footerPanel = new System.Windows.Forms.Panel();
            this.progressBar = new System.Windows.Forms.ProgressBar();
            this.lblProgress = new System.Windows.Forms.Label();
            this.lblProjectHeading = new System.Windows.Forms.Label();
            this.cmbProject = new System.Windows.Forms.ComboBox();
            this.chkAutoPatch = new System.Windows.Forms.CheckBox();
            this.chkAutoPlay = new System.Windows.Forms.CheckBox();
            this.btnVerifyFiles = new System.Windows.Forms.Button();
            this.btnCheck = new System.Windows.Forms.Button();
            this.btnStart = new System.Windows.Forms.Button();
            this.pendingPatchTimer = new System.Windows.Forms.Timer(this.components);
            this.mainShell.SuspendLayout();
            this.heroPanel.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.splashLogo)).BeginInit();
            this.sidebarPanel.SuspendLayout();
            this.contentPanel.SuspendLayout();
            this.headerPanel.SuspendLayout();
            this.tabContent.SuspendLayout();
            this.tabPatchNotes.SuspendLayout();
            this.tabPatchLog.SuspendLayout();
            this.footerPanel.SuspendLayout();
            this.SuspendLayout();
            //
            // mainShell
            //
            this.mainShell.ColumnCount = 2;
            this.mainShell.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 292F));
            this.mainShell.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.mainShell.Controls.Add(this.heroPanel, 0, 0);
            this.mainShell.Controls.Add(this.contentPanel, 1, 0);
            this.mainShell.Dock = System.Windows.Forms.DockStyle.Fill;
            this.mainShell.Location = new System.Drawing.Point(0, 0);
            this.mainShell.Name = "mainShell";
            this.mainShell.RowCount = 1;
            this.mainShell.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.mainShell.Size = new System.Drawing.Size(960, 640);
            this.mainShell.TabIndex = 0;
            //
            // heroPanel
            //
            this.heroPanel.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(12)))), ((int)(((byte)(17)))), ((int)(((byte)(24)))));
            this.heroPanel.Controls.Add(this.splashLogo);
            this.heroPanel.Controls.Add(this.lblHeroKicker);
            this.heroPanel.Controls.Add(this.lblHeroTitle);
            this.heroPanel.Controls.Add(this.lblHeroSubtitle);
            this.heroPanel.Controls.Add(this.sidebarPanel);
            this.heroPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.heroPanel.Location = new System.Drawing.Point(0, 0);
            this.heroPanel.Margin = new System.Windows.Forms.Padding(0);
            this.heroPanel.Name = "heroPanel";
            this.heroPanel.Size = new System.Drawing.Size(292, 640);
            this.heroPanel.TabIndex = 0;
            this.heroPanel.Paint += new System.Windows.Forms.PaintEventHandler(this.heroPanel_Paint);
            //
            // splashLogo
            //
            this.splashLogo.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.splashLogo.Image = global::EQEmu_Patcher.Properties.Resources.rof;
            this.splashLogo.Location = new System.Drawing.Point(24, 30);
            this.splashLogo.Name = "splashLogo";
            this.splashLogo.Size = new System.Drawing.Size(244, 150);
            this.splashLogo.SizeMode = System.Windows.Forms.PictureBoxSizeMode.Zoom;
            this.splashLogo.TabIndex = 0;
            this.splashLogo.TabStop = false;
            //
            // lblHeroKicker
            //
            this.lblHeroKicker.AutoSize = true;
            this.lblHeroKicker.BackColor = System.Drawing.Color.Transparent;
            this.lblHeroKicker.Font = new System.Drawing.Font("Segoe UI Semibold", 9.75F, System.Drawing.FontStyle.Bold);
            this.lblHeroKicker.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(224)))), ((int)(((byte)(181)))), ((int)(((byte)(92)))));
            this.lblHeroKicker.Location = new System.Drawing.Point(24, 205);
            this.lblHeroKicker.Name = "lblHeroKicker";
            this.lblHeroKicker.Size = new System.Drawing.Size(111, 17);
            this.lblHeroKicker.TabIndex = 1;
            this.lblHeroKicker.Text = "LIVE PATCH FEED";
            //
            // lblHeroTitle
            //
            this.lblHeroTitle.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.lblHeroTitle.BackColor = System.Drawing.Color.Transparent;
            this.lblHeroTitle.Font = new System.Drawing.Font("Segoe UI", 22F, System.Drawing.FontStyle.Bold);
            this.lblHeroTitle.ForeColor = System.Drawing.Color.White;
            this.lblHeroTitle.Location = new System.Drawing.Point(22, 226);
            this.lblHeroTitle.Name = "lblHeroTitle";
            this.lblHeroTitle.Size = new System.Drawing.Size(246, 96);
            this.lblHeroTitle.TabIndex = 2;
            this.lblHeroTitle.Text = "EQEmu Patcher";
            //
            // lblHeroSubtitle
            //
            this.lblHeroSubtitle.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.lblHeroSubtitle.BackColor = System.Drawing.Color.Transparent;
            this.lblHeroSubtitle.Font = new System.Drawing.Font("Segoe UI", 9.75F);
            this.lblHeroSubtitle.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(196)))), ((int)(((byte)(205)))), ((int)(((byte)(218)))));
            this.lblHeroSubtitle.Location = new System.Drawing.Point(25, 322);
            this.lblHeroSubtitle.Name = "lblHeroSubtitle";
            this.lblHeroSubtitle.Size = new System.Drawing.Size(239, 54);
            this.lblHeroSubtitle.TabIndex = 3;
            this.lblHeroSubtitle.Text = "Stay current, read the latest update notes, and launch directly into Norrath.";
            //
            // sidebarPanel
            //
            this.sidebarPanel.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.sidebarPanel.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(28)))), ((int)(((byte)(35)))), ((int)(((byte)(47)))));
            this.sidebarPanel.Controls.Add(this.lblServiceHeading);
            this.sidebarPanel.Controls.Add(this.lblServiceStatus);
            this.sidebarPanel.Controls.Add(this.lblServiceMessage);
            this.sidebarPanel.Controls.Add(this.lblClientHeading);
            this.sidebarPanel.Controls.Add(this.lblClientValue);
            this.sidebarPanel.Controls.Add(this.lblManifestHeading);
            this.sidebarPanel.Controls.Add(this.lblManifestValue);
            this.sidebarPanel.Controls.Add(this.lblDownloadHeading);
            this.sidebarPanel.Controls.Add(this.lblDownloadValue);
            this.sidebarPanel.Controls.Add(this.lblLastPatchHeading);
            this.sidebarPanel.Controls.Add(this.lblLastPatchValue);
            this.sidebarPanel.Location = new System.Drawing.Point(18, 405);
            this.sidebarPanel.Name = "sidebarPanel";
            this.sidebarPanel.Size = new System.Drawing.Size(256, 214);
            this.sidebarPanel.TabIndex = 4;
            //
            // lblServiceHeading
            //
            this.lblServiceHeading.AutoSize = true;
            this.lblServiceHeading.Font = new System.Drawing.Font("Segoe UI Semibold", 8.25F, System.Drawing.FontStyle.Bold);
            this.lblServiceHeading.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(134)))), ((int)(((byte)(148)))), ((int)(((byte)(166)))));
            this.lblServiceHeading.Location = new System.Drawing.Point(16, 14);
            this.lblServiceHeading.Name = "lblServiceHeading";
            this.lblServiceHeading.Size = new System.Drawing.Size(86, 13);
            this.lblServiceHeading.TabIndex = 0;
            this.lblServiceHeading.Text = "PATCH SERVICE";
            //
            // lblServiceStatus
            //
            this.lblServiceStatus.Font = new System.Drawing.Font("Segoe UI", 14F, System.Drawing.FontStyle.Bold);
            this.lblServiceStatus.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(116)))), ((int)(((byte)(226)))), ((int)(((byte)(158)))));
            this.lblServiceStatus.Location = new System.Drawing.Point(14, 28);
            this.lblServiceStatus.Name = "lblServiceStatus";
            this.lblServiceStatus.Size = new System.Drawing.Size(224, 28);
            this.lblServiceStatus.TabIndex = 1;
            this.lblServiceStatus.Text = "Checking...";
            //
            // lblServiceMessage
            //
            this.lblServiceMessage.Font = new System.Drawing.Font("Segoe UI", 8.75F);
            this.lblServiceMessage.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(196)))), ((int)(((byte)(205)))), ((int)(((byte)(218)))));
            this.lblServiceMessage.Location = new System.Drawing.Point(17, 57);
            this.lblServiceMessage.Name = "lblServiceMessage";
            this.lblServiceMessage.Size = new System.Drawing.Size(222, 36);
            this.lblServiceMessage.TabIndex = 2;
            this.lblServiceMessage.Text = "Fetching service status.";
            //
            // lblClientHeading
            //
            this.lblClientHeading.AutoSize = true;
            this.lblClientHeading.Font = new System.Drawing.Font("Segoe UI Semibold", 8.25F, System.Drawing.FontStyle.Bold);
            this.lblClientHeading.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(134)))), ((int)(((byte)(148)))), ((int)(((byte)(166)))));
            this.lblClientHeading.Location = new System.Drawing.Point(18, 103);
            this.lblClientHeading.Name = "lblClientHeading";
            this.lblClientHeading.Size = new System.Drawing.Size(42, 13);
            this.lblClientHeading.TabIndex = 3;
            this.lblClientHeading.Text = "CLIENT";
            //
            // lblClientValue
            //
            this.lblClientValue.Font = new System.Drawing.Font("Segoe UI Semibold", 9.75F, System.Drawing.FontStyle.Bold);
            this.lblClientValue.ForeColor = System.Drawing.Color.White;
            this.lblClientValue.Location = new System.Drawing.Point(94, 99);
            this.lblClientValue.Name = "lblClientValue";
            this.lblClientValue.Size = new System.Drawing.Size(145, 21);
            this.lblClientValue.TabIndex = 4;
            this.lblClientValue.Text = "Detecting";
            this.lblClientValue.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            //
            // lblManifestHeading
            //
            this.lblManifestHeading.AutoSize = true;
            this.lblManifestHeading.Font = new System.Drawing.Font("Segoe UI Semibold", 8.25F, System.Drawing.FontStyle.Bold);
            this.lblManifestHeading.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(134)))), ((int)(((byte)(148)))), ((int)(((byte)(166)))));
            this.lblManifestHeading.Location = new System.Drawing.Point(18, 128);
            this.lblManifestHeading.Name = "lblManifestHeading";
            this.lblManifestHeading.Size = new System.Drawing.Size(58, 13);
            this.lblManifestHeading.TabIndex = 5;
            this.lblManifestHeading.Text = "MANIFEST";
            //
            // lblManifestValue
            //
            this.lblManifestValue.Font = new System.Drawing.Font("Segoe UI Semibold", 9.75F, System.Drawing.FontStyle.Bold);
            this.lblManifestValue.ForeColor = System.Drawing.Color.White;
            this.lblManifestValue.Location = new System.Drawing.Point(94, 124);
            this.lblManifestValue.Name = "lblManifestValue";
            this.lblManifestValue.Size = new System.Drawing.Size(145, 21);
            this.lblManifestValue.TabIndex = 6;
            this.lblManifestValue.Text = "Pending";
            this.lblManifestValue.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            //
            // lblDownloadHeading
            //
            this.lblDownloadHeading.AutoSize = true;
            this.lblDownloadHeading.Font = new System.Drawing.Font("Segoe UI Semibold", 8.25F, System.Drawing.FontStyle.Bold);
            this.lblDownloadHeading.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(134)))), ((int)(((byte)(148)))), ((int)(((byte)(166)))));
            this.lblDownloadHeading.Location = new System.Drawing.Point(18, 153);
            this.lblDownloadHeading.Name = "lblDownloadHeading";
            this.lblDownloadHeading.Size = new System.Drawing.Size(66, 13);
            this.lblDownloadHeading.TabIndex = 7;
            this.lblDownloadHeading.Text = "PATCH SIZE";
            //
            // lblDownloadValue
            //
            this.lblDownloadValue.Font = new System.Drawing.Font("Segoe UI Semibold", 9.75F, System.Drawing.FontStyle.Bold);
            this.lblDownloadValue.ForeColor = System.Drawing.Color.White;
            this.lblDownloadValue.Location = new System.Drawing.Point(94, 149);
            this.lblDownloadValue.Name = "lblDownloadValue";
            this.lblDownloadValue.Size = new System.Drawing.Size(145, 21);
            this.lblDownloadValue.TabIndex = 8;
            this.lblDownloadValue.Text = "Pending";
            this.lblDownloadValue.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            //
            // lblLastPatchHeading
            //
            this.lblLastPatchHeading.AutoSize = true;
            this.lblLastPatchHeading.Font = new System.Drawing.Font("Segoe UI Semibold", 8.25F, System.Drawing.FontStyle.Bold);
            this.lblLastPatchHeading.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(134)))), ((int)(((byte)(148)))), ((int)(((byte)(166)))));
            this.lblLastPatchHeading.Location = new System.Drawing.Point(18, 178);
            this.lblLastPatchHeading.Name = "lblLastPatchHeading";
            this.lblLastPatchHeading.Size = new System.Drawing.Size(65, 13);
            this.lblLastPatchHeading.TabIndex = 9;
            this.lblLastPatchHeading.Text = "LAST PATCH";
            //
            // lblLastPatchValue
            //
            this.lblLastPatchValue.Font = new System.Drawing.Font("Segoe UI Semibold", 9.75F, System.Drawing.FontStyle.Bold);
            this.lblLastPatchValue.ForeColor = System.Drawing.Color.White;
            this.lblLastPatchValue.Location = new System.Drawing.Point(94, 174);
            this.lblLastPatchValue.Name = "lblLastPatchValue";
            this.lblLastPatchValue.Size = new System.Drawing.Size(145, 21);
            this.lblLastPatchValue.TabIndex = 10;
            this.lblLastPatchValue.Text = "Not patched";
            this.lblLastPatchValue.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            //
            // contentPanel
            //
            this.contentPanel.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(17)))), ((int)(((byte)(23)))), ((int)(((byte)(32)))));
            this.contentPanel.Controls.Add(this.headerPanel);
            this.contentPanel.Controls.Add(this.tabContent);
            this.contentPanel.Controls.Add(this.footerPanel);
            this.contentPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.contentPanel.Location = new System.Drawing.Point(292, 0);
            this.contentPanel.Margin = new System.Windows.Forms.Padding(0);
            this.contentPanel.Name = "contentPanel";
            this.contentPanel.Padding = new System.Windows.Forms.Padding(24);
            this.contentPanel.Size = new System.Drawing.Size(668, 640);
            this.contentPanel.TabIndex = 1;
            //
            // headerPanel
            //
            this.headerPanel.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.headerPanel.Controls.Add(this.lblTitle);
            this.headerPanel.Controls.Add(this.lblSubtitle);
            this.headerPanel.Controls.Add(this.lblStatusBadge);
            this.headerPanel.Controls.Add(this.lblVersion);
            this.headerPanel.Location = new System.Drawing.Point(24, 23);
            this.headerPanel.Name = "headerPanel";
            this.headerPanel.Size = new System.Drawing.Size(620, 88);
            this.headerPanel.TabIndex = 0;
            //
            // lblTitle
            //
            this.lblTitle.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.lblTitle.Font = new System.Drawing.Font("Segoe UI", 21F, System.Drawing.FontStyle.Bold);
            this.lblTitle.ForeColor = System.Drawing.Color.White;
            this.lblTitle.Location = new System.Drawing.Point(0, 0);
            this.lblTitle.Name = "lblTitle";
            this.lblTitle.Size = new System.Drawing.Size(414, 42);
            this.lblTitle.TabIndex = 0;
            this.lblTitle.Text = "EQEmu Patcher";
            //
            // lblSubtitle
            //
            this.lblSubtitle.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.lblSubtitle.Font = new System.Drawing.Font("Segoe UI", 9.75F);
            this.lblSubtitle.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(170)))), ((int)(((byte)(181)))), ((int)(((byte)(196)))));
            this.lblSubtitle.Location = new System.Drawing.Point(3, 44);
            this.lblSubtitle.Name = "lblSubtitle";
            this.lblSubtitle.Size = new System.Drawing.Size(517, 23);
            this.lblSubtitle.TabIndex = 1;
            this.lblSubtitle.Text = "Preparing the latest client files.";
            //
            // lblStatusBadge
            //
            this.lblStatusBadge.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.lblStatusBadge.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(51)))), ((int)(((byte)(77)))), ((int)(((byte)(108)))));
            this.lblStatusBadge.Font = new System.Drawing.Font("Segoe UI Semibold", 9.75F, System.Drawing.FontStyle.Bold);
            this.lblStatusBadge.ForeColor = System.Drawing.Color.White;
            this.lblStatusBadge.Location = new System.Drawing.Point(454, 7);
            this.lblStatusBadge.Name = "lblStatusBadge";
            this.lblStatusBadge.Size = new System.Drawing.Size(163, 30);
            this.lblStatusBadge.TabIndex = 2;
            this.lblStatusBadge.Text = "Checking";
            this.lblStatusBadge.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            //
            // lblVersion
            //
            this.lblVersion.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.lblVersion.Font = new System.Drawing.Font("Segoe UI", 8.25F);
            this.lblVersion.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(134)))), ((int)(((byte)(148)))), ((int)(((byte)(166)))));
            this.lblVersion.Location = new System.Drawing.Point(454, 43);
            this.lblVersion.Name = "lblVersion";
            this.lblVersion.Size = new System.Drawing.Size(163, 21);
            this.lblVersion.TabIndex = 3;
            this.lblVersion.Text = "Version";
            this.lblVersion.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            //
            // tabContent
            //
            this.tabContent.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
            | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.tabContent.Controls.Add(this.tabPatchNotes);
            this.tabContent.Controls.Add(this.tabPatchLog);
            this.tabContent.Location = new System.Drawing.Point(24, 120);
            this.tabContent.Name = "tabContent";
            this.tabContent.SelectedIndex = 0;
            this.tabContent.Size = new System.Drawing.Size(620, 395);
            this.tabContent.TabIndex = 1;
            //
            // tabPatchNotes
            //
            this.tabPatchNotes.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(22)))), ((int)(((byte)(29)))), ((int)(((byte)(40)))));
            this.tabPatchNotes.Controls.Add(this.txtPatchNotes);
            this.tabPatchNotes.Location = new System.Drawing.Point(4, 26);
            this.tabPatchNotes.Name = "tabPatchNotes";
            this.tabPatchNotes.Padding = new System.Windows.Forms.Padding(10);
            this.tabPatchNotes.Size = new System.Drawing.Size(612, 365);
            this.tabPatchNotes.TabIndex = 0;
            this.tabPatchNotes.Text = "Patch Notes";
            //
            // txtPatchNotes
            //
            this.txtPatchNotes.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(22)))), ((int)(((byte)(29)))), ((int)(((byte)(40)))));
            this.txtPatchNotes.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.txtPatchNotes.Dock = System.Windows.Forms.DockStyle.Fill;
            this.txtPatchNotes.Font = new System.Drawing.Font("Segoe UI", 10F);
            this.txtPatchNotes.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(225)))), ((int)(((byte)(231)))), ((int)(((byte)(239)))));
            this.txtPatchNotes.Location = new System.Drawing.Point(10, 10);
            this.txtPatchNotes.Name = "txtPatchNotes";
            this.txtPatchNotes.ReadOnly = true;
            this.txtPatchNotes.Size = new System.Drawing.Size(592, 345);
            this.txtPatchNotes.TabIndex = 0;
            this.txtPatchNotes.Text = "";
            //
            // tabPatchLog
            //
            this.tabPatchLog.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(22)))), ((int)(((byte)(29)))), ((int)(((byte)(40)))));
            this.tabPatchLog.Controls.Add(this.txtList);
            this.tabPatchLog.Location = new System.Drawing.Point(4, 26);
            this.tabPatchLog.Name = "tabPatchLog";
            this.tabPatchLog.Padding = new System.Windows.Forms.Padding(10);
            this.tabPatchLog.Size = new System.Drawing.Size(612, 365);
            this.tabPatchLog.TabIndex = 1;
            this.tabPatchLog.Text = "Patch Log";
            //
            // txtList
            //
            this.txtList.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(12)))), ((int)(((byte)(17)))), ((int)(((byte)(24)))));
            this.txtList.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.txtList.Dock = System.Windows.Forms.DockStyle.Fill;
            this.txtList.Font = new System.Drawing.Font("Consolas", 9.75F);
            this.txtList.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(210)))), ((int)(((byte)(218)))), ((int)(((byte)(230)))));
            this.txtList.HideSelection = false;
            this.txtList.Location = new System.Drawing.Point(10, 10);
            this.txtList.Multiline = true;
            this.txtList.Name = "txtList";
            this.txtList.ReadOnly = true;
            this.txtList.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.txtList.Size = new System.Drawing.Size(592, 345);
            this.txtList.TabIndex = 0;
            //
            // footerPanel
            //
            this.footerPanel.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.footerPanel.Controls.Add(this.lblProjectHeading);
            this.footerPanel.Controls.Add(this.cmbProject);
            this.footerPanel.Controls.Add(this.progressBar);
            this.footerPanel.Controls.Add(this.lblProgress);
            this.footerPanel.Controls.Add(this.chkAutoPatch);
            this.footerPanel.Controls.Add(this.chkAutoPlay);
            this.footerPanel.Controls.Add(this.btnVerifyFiles);
            this.footerPanel.Controls.Add(this.btnCheck);
            this.footerPanel.Controls.Add(this.btnStart);
            this.footerPanel.Location = new System.Drawing.Point(24, 528);
            this.footerPanel.Name = "footerPanel";
            this.footerPanel.Size = new System.Drawing.Size(620, 88);
            this.footerPanel.TabIndex = 2;
            //
            // progressBar
            //
            this.progressBar.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.progressBar.Location = new System.Drawing.Point(0, 8);
            this.progressBar.Name = "progressBar";
            this.progressBar.Size = new System.Drawing.Size(392, 14);
            this.progressBar.TabIndex = 0;
            //
            // lblProgress
            //
            this.lblProgress.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.lblProgress.Font = new System.Drawing.Font("Segoe UI", 8.25F);
            this.lblProgress.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(170)))), ((int)(((byte)(181)))), ((int)(((byte)(196)))));
            this.lblProgress.Location = new System.Drawing.Point(398, 4);
            this.lblProgress.Name = "lblProgress";
            this.lblProgress.Size = new System.Drawing.Size(74, 22);
            this.lblProgress.TabIndex = 1;
            this.lblProgress.Text = "0%";
            this.lblProgress.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            //
            // lblProjectHeading
            //
            this.lblProjectHeading.AutoSize = true;
            this.lblProjectHeading.Font = new System.Drawing.Font("Segoe UI Semibold", 8.25F, System.Drawing.FontStyle.Bold);
            this.lblProjectHeading.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(134)))), ((int)(((byte)(148)))), ((int)(((byte)(166)))));
            this.lblProjectHeading.Location = new System.Drawing.Point(0, 34);
            this.lblProjectHeading.Name = "lblProjectHeading";
            this.lblProjectHeading.Size = new System.Drawing.Size(88, 13);
            this.lblProjectHeading.TabIndex = 2;
            this.lblProjectHeading.Text = "TEST PROJECT";
            //
            // cmbProject
            //
            this.cmbProject.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(22)))), ((int)(((byte)(29)))), ((int)(((byte)(40)))));
            this.cmbProject.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cmbProject.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.cmbProject.Font = new System.Drawing.Font("Segoe UI", 9.75F);
            this.cmbProject.ForeColor = System.Drawing.Color.White;
            this.cmbProject.FormattingEnabled = true;
            this.cmbProject.Location = new System.Drawing.Point(0, 52);
            this.cmbProject.Name = "cmbProject";
            this.cmbProject.Size = new System.Drawing.Size(270, 25);
            this.cmbProject.TabIndex = 3;
            this.cmbProject.SelectedIndexChanged += new System.EventHandler(this.cmbProject_SelectedIndexChanged);
            //
            // chkAutoPatch
            //
            this.chkAutoPatch.AutoSize = true;
            this.chkAutoPatch.Font = new System.Drawing.Font("Segoe UI", 9F);
            this.chkAutoPatch.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(196)))), ((int)(((byte)(205)))), ((int)(((byte)(218)))));
            this.chkAutoPatch.Location = new System.Drawing.Point(288, 43);
            this.chkAutoPatch.Name = "chkAutoPatch";
            this.chkAutoPatch.Size = new System.Drawing.Size(86, 19);
            this.chkAutoPatch.TabIndex = 4;
            this.chkAutoPatch.Text = "Auto Patch";
            this.chkAutoPatch.UseVisualStyleBackColor = true;
            this.chkAutoPatch.CheckedChanged += new System.EventHandler(this.chkAutoPatch_CheckedChanged);
            //
            // chkAutoPlay
            //
            this.chkAutoPlay.AutoSize = true;
            this.chkAutoPlay.Font = new System.Drawing.Font("Segoe UI", 9F);
            this.chkAutoPlay.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(196)))), ((int)(((byte)(205)))), ((int)(((byte)(218)))));
            this.chkAutoPlay.Location = new System.Drawing.Point(288, 64);
            this.chkAutoPlay.Name = "chkAutoPlay";
            this.chkAutoPlay.Size = new System.Drawing.Size(76, 19);
            this.chkAutoPlay.TabIndex = 5;
            this.chkAutoPlay.Text = "Auto Play";
            this.chkAutoPlay.UseVisualStyleBackColor = true;
            this.chkAutoPlay.CheckedChanged += new System.EventHandler(this.chkAutoPlay_CheckedChanged);
            //
            // btnVerifyFiles
            //
            this.btnVerifyFiles.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnVerifyFiles.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(51)))), ((int)(((byte)(77)))), ((int)(((byte)(108)))));
            this.btnVerifyFiles.FlatAppearance.BorderSize = 0;
            this.btnVerifyFiles.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btnVerifyFiles.Font = new System.Drawing.Font("Segoe UI Semibold", 9F, System.Drawing.FontStyle.Bold);
            this.btnVerifyFiles.ForeColor = System.Drawing.Color.White;
            this.btnVerifyFiles.Location = new System.Drawing.Point(388, 37);
            this.btnVerifyFiles.Name = "btnVerifyFiles";
            this.btnVerifyFiles.Size = new System.Drawing.Size(108, 42);
            this.btnVerifyFiles.TabIndex = 6;
            this.btnVerifyFiles.Text = "Verify";
            this.btnVerifyFiles.UseVisualStyleBackColor = false;
            this.btnVerifyFiles.Click += new System.EventHandler(this.btnVerifyFiles_Click);
            //
            // btnCheck
            //
            this.btnCheck.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnCheck.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(203)))), ((int)(((byte)(154)))), ((int)(((byte)(70)))));
            this.btnCheck.FlatAppearance.BorderSize = 0;
            this.btnCheck.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btnCheck.Font = new System.Drawing.Font("Segoe UI Semibold", 10F, System.Drawing.FontStyle.Bold);
            this.btnCheck.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(20)))), ((int)(((byte)(16)))), ((int)(((byte)(11)))));
            this.btnCheck.Location = new System.Drawing.Point(506, 37);
            this.btnCheck.Name = "btnCheck";
            this.btnCheck.Size = new System.Drawing.Size(114, 42);
            this.btnCheck.TabIndex = 7;
            this.btnCheck.Text = "Checking...";
            this.btnCheck.UseVisualStyleBackColor = false;
            this.btnCheck.Click += new System.EventHandler(this.btnCheck_Click);
            //
            // btnStart
            //
            this.btnStart.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnStart.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(79)))), ((int)(((byte)(185)))), ((int)(((byte)(119)))));
            this.btnStart.FlatAppearance.BorderSize = 0;
            this.btnStart.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btnStart.Font = new System.Drawing.Font("Segoe UI Semibold", 10F, System.Drawing.FontStyle.Bold);
            this.btnStart.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(8)))), ((int)(((byte)(20)))), ((int)(((byte)(13)))));
            this.btnStart.Location = new System.Drawing.Point(512, 37);
            this.btnStart.Name = "btnStart";
            this.btnStart.Size = new System.Drawing.Size(108, 42);
            this.btnStart.TabIndex = 8;
            this.btnStart.Text = "Play";
            this.btnStart.UseVisualStyleBackColor = false;
            this.btnStart.Visible = false;
            this.btnStart.Click += new System.EventHandler(this.btnStart_Click);
            //
            // pendingPatchTimer
            //
            this.pendingPatchTimer.Tick += new System.EventHandler(this.pendingPatchTimer_Tick);
            //
            // MainForm
            //
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 17F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(17)))), ((int)(((byte)(23)))), ((int)(((byte)(32)))));
            this.ClientSize = new System.Drawing.Size(960, 640);
            this.Controls.Add(this.mainShell);
            this.Font = new System.Drawing.Font("Segoe UI", 9.75F);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimumSize = new System.Drawing.Size(860, 560);
            this.Name = "MainForm";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "EQEmu Patcher";
            this.Load += new System.EventHandler(this.MainForm_Load);
            this.Shown += new System.EventHandler(this.MainForm_Shown);
            this.mainShell.ResumeLayout(false);
            this.heroPanel.ResumeLayout(false);
            this.heroPanel.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.splashLogo)).EndInit();
            this.sidebarPanel.ResumeLayout(false);
            this.sidebarPanel.PerformLayout();
            this.contentPanel.ResumeLayout(false);
            this.headerPanel.ResumeLayout(false);
            this.tabContent.ResumeLayout(false);
            this.tabPatchNotes.ResumeLayout(false);
            this.tabPatchLog.ResumeLayout(false);
            this.tabPatchLog.PerformLayout();
            this.footerPanel.ResumeLayout(false);
            this.footerPanel.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TableLayoutPanel mainShell;
        private System.Windows.Forms.Panel heroPanel;
        private System.Windows.Forms.PictureBox splashLogo;
        private System.Windows.Forms.Label lblHeroKicker;
        private System.Windows.Forms.Label lblHeroTitle;
        private System.Windows.Forms.Label lblHeroSubtitle;
        private System.Windows.Forms.Panel sidebarPanel;
        private System.Windows.Forms.Label lblServiceHeading;
        private System.Windows.Forms.Label lblServiceStatus;
        private System.Windows.Forms.Label lblServiceMessage;
        private System.Windows.Forms.Label lblClientHeading;
        private System.Windows.Forms.Label lblClientValue;
        private System.Windows.Forms.Label lblManifestHeading;
        private System.Windows.Forms.Label lblManifestValue;
        private System.Windows.Forms.Label lblDownloadHeading;
        private System.Windows.Forms.Label lblDownloadValue;
        private System.Windows.Forms.Label lblLastPatchHeading;
        private System.Windows.Forms.Label lblLastPatchValue;
        private System.Windows.Forms.Panel contentPanel;
        private System.Windows.Forms.Panel headerPanel;
        private System.Windows.Forms.Label lblTitle;
        private System.Windows.Forms.Label lblSubtitle;
        private System.Windows.Forms.Label lblStatusBadge;
        private System.Windows.Forms.Label lblVersion;
        private System.Windows.Forms.TabControl tabContent;
        private System.Windows.Forms.TabPage tabPatchNotes;
        private System.Windows.Forms.RichTextBox txtPatchNotes;
        private System.Windows.Forms.TabPage tabPatchLog;
        private System.Windows.Forms.TextBox txtList;
        private System.Windows.Forms.Panel footerPanel;
        private System.Windows.Forms.ProgressBar progressBar;
        private System.Windows.Forms.Label lblProgress;
        private System.Windows.Forms.Label lblProjectHeading;
        private System.Windows.Forms.ComboBox cmbProject;
        private System.Windows.Forms.CheckBox chkAutoPatch;
        private System.Windows.Forms.CheckBox chkAutoPlay;
        private System.Windows.Forms.Button btnVerifyFiles;
        private System.Windows.Forms.Button btnCheck;
        private System.Windows.Forms.Button btnStart;
        private System.Windows.Forms.Timer pendingPatchTimer;
    }
}
