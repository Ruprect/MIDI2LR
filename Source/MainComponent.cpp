/*

*/

#include "MainComponent.h"

//==============================================================================
MainContentComponent::MainContentComponent() : _titleLabel("Title", "MIDI2LR"),
                                               _connectionLabel("Connection", "Not connected to LR"),
                                               _commandLabel("Command", ""),
                                               _commandTable("Table", nullptr),
                                               _commandTableModel(),
                                               _rescanButton("Rescan MIDI devices"),
                                               _removeRowButton("Remove selected row"),
                                               _saveButton("Save"),
                                               _loadButton("Load"),
                                               _versionLabel("Version", "Version " +
                                                                        String(ProjectInfo::versionString))
{
    // Main title
    _titleLabel.setFont(Font(36.f, Font::bold));
    _titleLabel.setEditable(false);
    _titleLabel.setColour(Label::textColourId, Colours::darkgrey);
    _titleLabel.setComponentEffect(&_titleShadow);
    addAndMakeVisible(_titleLabel);

    // Connection status
    _connectionLabel.setFont(Font(12.f, Font::bold));
    _connectionLabel.setEditable(false);
    _connectionLabel.setColour(Label::backgroundColourId, Colours::red);
    _connectionLabel.setColour(Label::textColourId, Colours::black);
    _connectionLabel.setJustificationType(Justification::centred);
    addAndMakeVisible(_connectionLabel);

    // Last MIDI command
    _commandLabel.setFont(Font(12.f, Font::bold));
    _commandLabel.setEditable(false);
    _commandLabel.setColour(Label::textColourId, Colours::darkgrey);
    addAndMakeVisible(_commandLabel);

    // Rescan MIDI button
    _rescanButton.addListener(this);
    addAndMakeVisible(_rescanButton);

    // Add ourselves as a listener for MIDI commands
    MIDIProcessor::getInstance().addMIDICommandListener(this);

    // Add ourselves as a listener for LR_IPC events
    LR_IPC::getInstance().addListener(this);

    // Command Table
    _commandTable.setModel(&_commandTableModel);
    addAndMakeVisible(_commandTable);

    // Remove row button
    _removeRowButton.addListener(this);
    addAndMakeVisible(_removeRowButton);

    // Save button
    _saveButton.addListener(this);
    addAndMakeVisible(_saveButton);

    // Load button
    _loadButton.addListener(this);
    addAndMakeVisible(_loadButton);

    // Version label
    _versionLabel.setFont(Font(12.f, Font::bold));
    _versionLabel.setEditable(false);
    _versionLabel.setColour(Label::textColourId, Colours::darkgrey);
    addAndMakeVisible(_versionLabel);

    setSize (400, 600);

    // Start LR IPC
    LR_IPC::getInstance();
     
}

MainContentComponent::~MainContentComponent()
{
}

void MainContentComponent::paint (Graphics& g)
{
    g.fillAll (Colours::white);
}

void MainContentComponent::resized()
{
    // top components
    _titleLabel.setBoundsRelative(.1f, 0.f, .5f, .15f);
    _connectionLabel.setBoundsRelative(.1f, .125f, .3f, .05f);
    _commandLabel.setBoundsRelative(.8f, 0.0375f, .2f, .075f);
    _rescanButton.setBoundsRelative(.8f, 0.1175f, .2f, .05f);

    // table
    _commandTable.setBoundsRelative(.1f, .2f, .8f, .6f);

    // bottom components
    _removeRowButton.setBoundsRelative(.1f, .85f, .4f, .05f);
    _saveButton.setBoundsRelative(.55f, .85f, .15f, .05f);
    _loadButton.setBoundsRelative(.75f, .85f, .15f, .05f);
    _versionLabel.setBoundsRelative(.8f, .9f, .2f, .1f);
}

// Update MIDI command components
void MainContentComponent::handleAsyncUpdate()
{
    // Update the last command label and set its colour to green
    _commandLabel.setText(_lastCommand, NotificationType::dontSendNotification);
    _commandLabel.setColour(Label::backgroundColourId, Colours::greenyellow);
    startTimer(1000);

    // Update the command table to add and/or select row corresponding to midi command
    _commandTable.updateContent();
    _commandTable.selectRow(_rowToSelect);
}

void MainContentComponent::handleMidiCC(int midiChannel, int controller, int value)
{
    _lastCommand = String::formatted("%d: CC%d [%d]", midiChannel, controller, value);
    _commandTableModel.addRow(midiChannel, controller, true);
    _rowToSelect = _commandTableModel.getRowForMessage(midiChannel, controller, true);
    triggerAsyncUpdate();
}

void MainContentComponent::handleMidiNote(int midiChannel, int note)
{
    _lastCommand = String::formatted("%d: Note [%d]", midiChannel, note);
    _commandTableModel.addRow(midiChannel, note, false);
    _rowToSelect = _commandTableModel.getRowForMessage(midiChannel, note, false);
    triggerAsyncUpdate();
}

void MainContentComponent::connected()
{
    _connectionLabel.setText("Connected to LR", NotificationType::dontSendNotification);
    _connectionLabel.setColour(Label::backgroundColourId, Colours::greenyellow);
}

void MainContentComponent::disconnected()
{
    _connectionLabel.setText("Not connected to LR", NotificationType::dontSendNotification);
    _connectionLabel.setColour(Label::backgroundColourId, Colours::red);
}

void MainContentComponent::timerCallback()
{
    // reset the command label's background to white
    _commandLabel.setColour(Label::backgroundColourId, Colours::white);
    stopTimer();
}

void MainContentComponent::buttonClicked(Button* button)
{
    if (button == &_rescanButton)
        MIDIProcessor::getInstance().rescanDevices();
    else if (button == &_removeRowButton)
    {
        if (_commandTable.getSelectedRow() != -1)
        {
            _commandTableModel.removeRow(_commandTable.getSelectedRow());
            _commandTable.updateContent();
        }
    }
    else if (button == &_saveButton)
    {
        FileBrowserComponent browser(FileBrowserComponent::canSelectFiles | FileBrowserComponent::saveMode | 
                                     FileBrowserComponent::warnAboutOverwriting,
                                     File::getCurrentWorkingDirectory(),
                                     nullptr,
                                     nullptr);
        FileChooserDialogBox dialogBox("Save profile",
                                       "Enter filename to save profile",
                                       browser,
                                       true,
                                       Colours::lightgrey);
        if (dialogBox.show())
        {
            File selectedFile = browser.getSelectedFile(0).withFileExtension("xml");
            CommandMap::getInstance().toXMLDocument(selectedFile);
        }
    }
    else if (button == &_loadButton)
    {
        WildcardFileFilter wildcardFilter("*.xml", String::empty, "MIDI2LR profiles");
        FileBrowserComponent browser(FileBrowserComponent::canSelectFiles | FileBrowserComponent::openMode,
                                     File::getCurrentWorkingDirectory(),
                                     &wildcardFilter,
                                     nullptr);
        FileChooserDialogBox dialogBox("Open profile",
                                       "Select a profile to open",
                                       browser,
                                       true,
                                       Colours::lightgrey);

        if (dialogBox.show())
        {
            ScopedPointer<XmlElement> elem = XmlDocument::parse(browser.getSelectedFile(0));
            if(elem)
            {
                _commandTableModel.buildFromXml(elem);
                _commandTable.updateContent();
            }
        }
    }
}